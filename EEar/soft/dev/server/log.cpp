
#include "include.h"



void CLog::Add(const char *format,...)
{
  OURTIME now = GetNowOurTime();

  char buffer[1024];
  va_list args;
  va_start(args,format);
  vsprintf(buffer,format,args);
  va_end(args);

  CSQLite sql(GetDBFilename().c_str(),DB_LOCK_WRITE_TIMEOUT_MS,FALSE);
  sql.TurnOffSyncWrite();  // speedup

  sql.Exec(L"CREATE TABLE IF NOT EXISTS TLog(ev_time INT,ev_desc TEXT)");
  sql.Exec(L"CREATE INDEX IF NOT EXISTS ILog ON TLog(ev_time)");

  CSQLiteQuery *q = sql.CreateQuery(L"INSERT INTO TLog VALUES(?,?)");
  q->BindAsInt64(now);
  q->BindAsText((const WCHAR*)CUnicode(buffer));
  q->Step();
  q->Destroy();

  char cmd[MAX_PATH];
  sprintf(cmd,"DELETE FROM TLog WHERE ev_time<%I64d",now-(OURTIME)RETROSPECTIVE_DAYS*OURTIME_1_DAY);
  sql.Exec(CUnicode(cmd));
}


BOOL CLog::SaveLog(const char *filename)
{
  FILE *f = fopen(filename,"wb");
  if ( f )
     {
       const unsigned short prefix = 0xFEFF;
       fwrite(&prefix,2,1,f);

       {
         CSQLite sql(GetDBFilename().c_str(),DB_LOCK_READ_TIMEOUT_MS,TRUE);

         CSQLiteQuery *q = sql.CreateQuery(L"SELECT ev_time,ev_desc FROM TLog ORDER BY ev_time DESC LIMIT 50000");

         BOOL is_data = FALSE;
         while ( q->Step(&is_data) && is_data )
         {
           std::wstring s_time = (const WCHAR*)CUnicode(OurTimeToString(q->GetAsInt64(0)).c_str());
           std::wstring s_desc = q->GetAsText(1);

           std::wstring line;
           line += s_time;
           line += L" ";
           line += s_desc;
           line += L"\r\n";

           fwrite(line.data(),line.size(),sizeof(WCHAR),f);
         }

         q->Destroy();
       }

       fclose(f);
     }

  return f != NULL;
}

