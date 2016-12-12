
#include "include.h"



void OurTimeToSystemTime(OURTIME d,SYSTEMTIME& st)
{
  const int n_year = 2000;
  const int n_month = 1;
  const int n_day = 1;
  const int n_hour = 0;
  const int n_min = 0;
  const int n_sec = 0;

  const SYSTEMTIME n_st = {n_year,n_month,0,n_day,n_hour,n_min,n_sec,0};

  __int64 n_ft = 0;

  SystemTimeToFileTime(&n_st,(FILETIME*)&n_ft);

  __int64 i_ft = n_ft + d*10000;

  FileTimeToSystemTime((const FILETIME*)&i_ft,&st);
}


OURTIME SystemTimeToOurTime(const SYSTEMTIME& st)
{
  int i_year = st.wYear;
  int i_month = st.wMonth;
  int i_day = st.wDay;
  int i_hour = st.wHour;
  int i_min = st.wMinute;
  int i_sec = st.wSecond;
  int i_ms = st.wMilliseconds;

  const int n_year = 2000;
  const int n_month = 1;
  const int n_day = 1;
  const int n_hour = 0;
  const int n_min = 0;
  const int n_sec = 0;
  const int n_ms = 0;

  const SYSTEMTIME i_st = {i_year,i_month,0,i_day,i_hour,i_min,i_sec,i_ms};
  const SYSTEMTIME n_st = {n_year,n_month,0,n_day,n_hour,n_min,n_sec,n_ms};

  __int64 i_ft = 0, n_ft = 0;

  SystemTimeToFileTime(&i_st,(FILETIME*)&i_ft);
  SystemTimeToFileTime(&n_st,(FILETIME*)&n_ft);

  return (i_ft - n_ft) / 10000;
}


OURTIME GetNowOurTime()
{
  SYSTEMTIME st;
  GetLocalTime(&st);

  return SystemTimeToOurTime(st);
}


OURTIME GetNowOurTimeUTC0()
{
  SYSTEMTIME st;
  GetSystemTime(&st);

  return SystemTimeToOurTime(st);
}


void OurTimeToString(char *s,OURTIME t)
{
  if ( s )
     {
       SYSTEMTIME st;
       OurTimeToSystemTime(t,st);
       
       sprintf(s,"%04d-%02d-%02d %02d:%02d:%02d.%03d",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
     }
}    


std::string OurTimeToString(OURTIME t)
{
  char s[64];
  OurTimeToString(s,t);
  return s;
}


