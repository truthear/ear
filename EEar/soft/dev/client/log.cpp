
#include "include.h"



CLog::CLog(const char *filename,bool stdout_echo)
{
  strcpy(m_filename,filename);
  b_stdout_echo = stdout_echo;
}


CLog::~CLog()
{
}


void CLog::Add(const char *format,...)
{
  char s[MAX_LOG_STR_SIZE];

  strcpy(s,"[");
  OurTimeToString(CRTC::GetTime(),s+strlen(s));
  strcat(s,"] ");

  va_list ap;
  va_start(ap, format);
  vsprintf(s+strlen(s), format, ap);
  va_end(ap);

  strcat(s,"\r\n");

  FIL f;
  if ( f_open(&f,m_filename,FA_WRITE|FA_OPEN_APPEND) == FR_OK )
     {
       UINT wb = 0;
       f_write(&f,s,strlen(s),&wb);
       f_close(&f);
     }

  if ( b_stdout_echo )
     {
       printf("%s",s);
     }
}



