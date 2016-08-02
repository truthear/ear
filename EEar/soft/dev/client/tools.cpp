
#include "include.h"




OURTIME ConvertOurTime(int yyyy,int mm,int dd,int hh,int nn,int ss,int msec)
{
  struct tm t;

  t.tm_sec = ss;
  t.tm_min = nn;
  t.tm_hour = hh;
  t.tm_mday = dd;
  t.tm_mon = mm-1;
  t.tm_year = yyyy-1900;
  t.tm_wday = 0;
  t.tm_yday = 0;
  t.tm_isdst = 0;

  time_t tt = mktime(&t);

  return (OURTIME)tt*1000+msec;
}


// not IRQ handler safe! (because of localtime())
char* OurTimeToString(OURTIME t,char *s)
{
  if ( s )
     {
       int msec = t % 1000;
       
       time_t tim = (time_t)(t / 1000);

       const struct tm* t2 = localtime(&tim);

       sprintf(s,"%04d-%02d-%02d %02d:%02d:%02d.%03d",t2->tm_year+1900,t2->tm_mon+1,t2->tm_mday,t2->tm_hour,t2->tm_min,t2->tm_sec,msec);
     }

  return s;
}


