
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include "sqlite3.h"
#include "sqlite.h"
#include "../tools.h"



CSQLiteQuery::CSQLiteQuery(CSQLiteDB *owner,sqlite3 *db,const WCHAR *query_fmt)
{
  p_owner = owner;
  p_db = db;
  p_stmt = NULL;
  m_curr_arg = 0;

  if ( p_db )
     {
       sqlite3_prepare16_v2(p_db,query_fmt,-1,&p_stmt,NULL);
     }
}


CSQLiteQuery::~CSQLiteQuery()
{
  if ( p_stmt )
     {
       sqlite3_finalize(p_stmt);
       p_stmt = NULL;
     }
}


void CSQLiteQuery::Destroy()
{
  delete this;
}


void CSQLiteQuery::Release()
{
  delete this;
}


BOOL CSQLiteQuery::BindAsNull()
{
  return p_stmt ? (sqlite3_bind_null(p_stmt,++m_curr_arg) == SQLITE_OK) : FALSE;
}


BOOL CSQLiteQuery::BindAsBlob(const void *buff,int size)
{
  return p_stmt ? (sqlite3_bind_blob(p_stmt,++m_curr_arg,buff,size,SQLITE_TRANSIENT) == SQLITE_OK) : FALSE;
}


BOOL CSQLiteQuery::BindAsDouble(double v)
{
  return p_stmt ? (sqlite3_bind_double(p_stmt,++m_curr_arg,v) == SQLITE_OK) : FALSE;
}


BOOL CSQLiteQuery::BindAsInt(int v)
{
  return p_stmt ? (sqlite3_bind_int(p_stmt,++m_curr_arg,v) == SQLITE_OK) : FALSE;
}


BOOL CSQLiteQuery::BindAsInt64(__int64 v)
{
  return p_stmt ? (sqlite3_bind_int64(p_stmt,++m_curr_arg,v) == SQLITE_OK) : FALSE;
}


BOOL CSQLiteQuery::BindAsText(const WCHAR *v)
{
  return p_stmt ? (sqlite3_bind_text16(p_stmt,++m_curr_arg,v,-1,SQLITE_TRANSIENT) == SQLITE_OK) : FALSE;
}


BOOL CSQLiteQuery::BindAsText(const std::wstring& v)
{
  return p_stmt ? (sqlite3_bind_text16(p_stmt,++m_curr_arg,v.c_str(),-1,SQLITE_TRANSIENT) == SQLITE_OK) : FALSE;
}


BOOL CSQLiteQuery::Step(BOOL *_is_data_avail)
{
  BOOL rc = FALSE;
  
  if ( _is_data_avail )
     {
       *_is_data_avail = FALSE;
     }

  if ( p_stmt )
     {
       int c = sqlite3_step(p_stmt);

       if ( c == SQLITE_DONE || c == SQLITE_ROW )
          {
            rc = TRUE;

            if ( _is_data_avail )
               {
                 *_is_data_avail = (c == SQLITE_ROW);
               }
          }
     }

  return rc;
}


BOOL CSQLiteQuery::Reset()
{
  return p_stmt ? (sqlite3_reset(p_stmt) == SQLITE_OK) : FALSE;
}


int CSQLiteQuery::GetNumRowsAffected()
{
  return p_owner->GetNumRowsAffected();
}


__int64 CSQLiteQuery::GetLastInsertRowId()
{
  return p_owner->GetLastInsertRowId();
}


int CSQLiteQuery::GetNumCols()
{
  return p_stmt ? sqlite3_column_count(p_stmt) : 0;
}


void CSQLiteQuery::GetAsBlob(int col,std::string& _blob)
{
  _blob.clear();
  
  if ( p_stmt )
     {
       const void *blob_ptr = sqlite3_column_blob(p_stmt,col);  // do not free it!
       int blob_size = sqlite3_column_bytes(p_stmt,col);  // must be called after sqlite3_column_blob()!

       if ( blob_ptr && blob_size > 0 )
          {
            _blob.assign((const char*)blob_ptr,(size_t)blob_size);
          }
     }
}


double CSQLiteQuery::GetAsDouble(int col)
{
  return p_stmt ? sqlite3_column_double(p_stmt,col) : 0.0;
}


int CSQLiteQuery::GetAsInt(int col)
{
  return p_stmt ? sqlite3_column_int(p_stmt,col) : 0;
}


__int64 CSQLiteQuery::GetAsInt64(int col)
{
  return p_stmt ? sqlite3_column_int64(p_stmt,col) : 0;
}


std::wstring CSQLiteQuery::GetAsText(int col)
{
  std::wstring rc;
  
  if ( p_stmt )
     {
       const WCHAR *text = (const WCHAR*)sqlite3_column_text16(p_stmt,col);  // do not free it!

       if ( text )
          {
            rc = text;
          }
     }

  return rc;
}


////////////////////////////


CSQLiteDB::CSQLiteDB(const WCHAR *filename,BOOL is_read_only)
{
  p_db = NULL;

  if ( !is_read_only )
     {
       sqlite3_open16(filename,&p_db);
     }
  else
     {
       sqlite3_open_v2(CUTF8Encoder(filename),&p_db,SQLITE_OPEN_READONLY,NULL);
     }
}


CSQLiteDB::~CSQLiteDB()
{
  if ( p_db )
     {
       sqlite3_close(p_db);
       p_db = NULL;
     }
}


CSQLiteQuery* CSQLiteDB::CreateQuery(const WCHAR *query_fmt)
{
  return new CSQLiteQuery(this,p_db,query_fmt);
}


int CSQLiteDB::GetNumRowsAffected()
{
  return p_db ? sqlite3_changes(p_db) : 0;
}


__int64 CSQLiteDB::GetLastInsertRowId()
{
  return p_db ? sqlite3_last_insert_rowid(p_db) : 0;
}


int CSQLiteDB::GetLastError()
{
  return p_db ? sqlite3_errcode(p_db) : 0;
}


/////////////////////////



CSQLite::CSQLite(const WCHAR *filename,unsigned wait_lock_timeout_ms,BOOL is_read_only)
   : CSQLiteDB(filename,is_read_only)
{
  sqlite3 *db = GetDBHandle();
  if ( db )
     {
       sqlite3_busy_timeout(db,wait_lock_timeout_ms);
     }
}


BOOL CSQLite::Exec(const WCHAR *query)
{
  CSQLiteQuery *q = CreateQuery(query);
  BOOL rc = q->Step();
  q->Destroy();
  return rc;
}


BOOL CSQLite::ShrinkDB()
{
  return Exec(L"VACUUM");
}


BOOL CSQLite::TurnOffSyncWrite()
{
  return Exec(L"PRAGMA synchronous=OFF");
}


BOOL CSQLite::TurnOffJournal()
{
  return Exec(L"PRAGMA journal_mode=OFF");
}


BOOL CSQLite::GetPragmaValue(const char *pragma,__int64& _value)
{
  BOOL rc = FALSE;
  
  char query[MAX_PATH];
  sprintf(query,"PRAGMA %s",pragma);
  
  CSQLiteQuery *q = CreateQuery(CUnicode(query));

  BOOL is_data = FALSE;
  if ( q->Step(&is_data) && is_data )
     {
       rc = TRUE;
       _value = q->GetAsInt64(0);
     }

  q->Destroy();

  return rc;
}


BOOL CSQLite::GetRealDBSize(__int64& _dbsize)
{
  BOOL rc = FALSE;

  __int64 page_size = 0;
  __int64 page_count = 0;
  __int64 freelist_count = 0;

  if ( GetPragmaValue("page_size",page_size) && 
       GetPragmaValue("page_count",page_count) && 
       GetPragmaValue("freelist_count",freelist_count) )
     {
       if ( page_size > 0 && page_count >= 0 && freelist_count >= 0 && page_count >= freelist_count )
          {
            rc = TRUE;
            _dbsize = (page_count - freelist_count) * page_size;
          }
     }

  return rc;
}



