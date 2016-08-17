
#ifndef __SQLITE_H__
#define __SQLITE_H__


struct sqlite3;
struct sqlite3_stmt;
class CSQLiteDB;
class CSQLiteQuery;


class CSQLiteQuery
{
          CSQLiteDB *p_owner;
          sqlite3 *p_db;
          sqlite3_stmt *p_stmt;
          int m_curr_arg;

  protected:
          friend class CSQLiteDB;

          CSQLiteQuery(CSQLiteDB *owner,sqlite3 *db,const WCHAR *query_fmt);

  private:
          ~CSQLiteQuery();

  public:
          void Destroy();
          void Release();  // same as Destroy()

          BOOL BindAsNull();
          BOOL BindAsBlob(const void *buff,int size);
          BOOL BindAsDouble(double v);
          BOOL BindAsInt(int v);
          BOOL BindAsInt64(__int64 v);
          BOOL BindAsText(const WCHAR *v);
          BOOL BindAsText(const std::wstring& v);
          BOOL Step(BOOL *_is_data_avail=NULL);
          BOOL Reset();  // reset dont change bindings!
          int GetNumRowsAffected();
          __int64 GetLastInsertRowId();
          int GetNumCols();
          void GetAsBlob(int col,std::string& _blob);
          double GetAsDouble(int col);
          int GetAsInt(int col);
          __int64 GetAsInt64(int col);
          std::wstring GetAsText(int col);
};


class CSQLiteDB
{
          sqlite3 *p_db;

  public:
          CSQLiteDB(const WCHAR *filename,BOOL is_read_only);
          ~CSQLiteDB();

          CSQLiteQuery* CreateQuery(const WCHAR *query_fmt);
          
          int GetNumRowsAffected();
          __int64 GetLastInsertRowId();

          int GetLastError();

  protected:
          sqlite3* GetDBHandle() const { return p_db; }
};


class CSQLite : public CSQLiteDB
{
  public:
          CSQLite(const WCHAR *filename,unsigned wait_lock_timeout_ms,BOOL is_read_only);

          BOOL Exec(const WCHAR *query);
          BOOL ShrinkDB();
          BOOL TurnOffSyncWrite();
          BOOL TurnOffJournal();
          BOOL GetPragmaValue(const char *pragma,__int64& _value);
          BOOL GetRealDBSize(__int64& _dbsize);
};


#endif
