
#include "include.h"



BOOL SearchParam(const char *s)
{
  for ( int n = 1; n < __argc; n++ )
      if ( !lstrcmpi(__argv[n],s) )
         return TRUE;

  return FALSE;
}


BOOL IsSilent()
{
  return SearchParam("-silent");
}


void Message(const char *s)
{
  if ( !IsSilent() )
     {
       printf("%s\n",s);
     }
}


int main()
{
  int rc = 0;
  
  CMyService oService;
  CServiceManager *sm = CServiceManager::Create(&oService);
  
  if ( SearchParam("-install") )
     {
       if ( sm->HighLevelInstall() )
          {
            Message("OK");
            rc = 1;
          }
       else
          {
            Message("failed");
          }
     }
  else
  if ( SearchParam("-uninstall") )
     {
       if ( sm->HighLevelUninstall() )
          {
            Message("OK");
            rc = 1;
          }
       else
          {
            Message("failed");
          }
     }
  else
  if ( SearchParam("-log") )
     {
       printf("Please wait...\n");
       
       char filename[MAX_PATH] = "";
       GetFileNameInTempDir("__ear_log.txt",filename,TRUE);
       if ( !IsStrEmpty(filename) )
          {
            DeleteFile(filename);
            CLog::SaveLog(filename);
            if ( IsFileExist(filename) )
               {
                 ShellExecute(NULL,NULL,filename,NULL,NULL,SW_SHOWNORMAL);
               }
          }
     }
  else
     {
       rc = sm->HighLevelProcess() ? 1 : 0;
     }

  sm->Release();
  return rc ? 0 : 1;
}
