
#include "include.h"




COurTime::COurTime(int yy,int mm,int dd,int hh,int nn,int ss,int msec)
{
  if ( yy < 0 || yy > 99 || mm < 1 || mm > 12 || dd < 1 || dd > 31 )
     {
       m_time = -1;
     }
  else
     {
       int num_leaps = (yy+3)/4; // for 21'th cent is true
       int days = (yy*365+num_leaps) + GetMonthTable(yy)[mm-1] + (dd-1);
       int msec_day_part = hh*3600000+nn*60000+ss*1000+msec;

       m_time = ((OURTIME)((unsigned)days*(unsigned)84375) << 10) + (OURTIME)msec_day_part;
     }
}


bool COurTime::Unpack(int& yy,int& mm,int& dd,int& hh,int& nn,int& ss,int& msec) const
{
  bool rc = false;
       
  if ( m_time >= 0 && m_time <= MAX_TIME_VALUE )
     {
       OURTIME t = m_time;
       
       msec = t % 1000;
       t /= 1000;  // now t in seconds

       ss = t % 60;
       t /= 60;    // now t in minutes

       nn = t % 60;
       t /= 60;    // now t in hours

       hh = t % 24;
       t /= 24;    // now t in days

       yy = 0;

       for ( int n = 0; n <= 99; n++ )
           {
             int days_in_year = (IsLeapYear(n) ? 366 : 365);
             
             t -= days_in_year;

             if ( t < 0 )
                {
                  t += days_in_year;
                  break;
                }

             yy++;
           }

       // t now is rest in days within year
       mm = 1; // to disable warning
       const int *mtable = GetMonthTable(yy);
       for ( int n = 11; n >= 0; n-- )
           {
             if ( t >= mtable[n] )
                {
                  mm = n+1;
                  t -= mtable[n];
                  break;
                }
           }

       // t now is days rest
       dd = t+1;

       rc = true;
     }

  return rc;
}


char* COurTime::ToString(char *s) const
{
  if ( s )
     {
       int yy,mm,dd,hh,nn,ss,msec;
       
       if ( Unpack(yy,mm,dd,hh,nn,ss,msec) )
          {
            sprintf(s,"%04d-%02d-%02d %02d:%02d:%02d.%03d",2000+yy,mm,dd,hh,nn,ss,msec);
          }
       else
          {
            sprintf(s,"%s","Invalid date/time");
          }
     }

  return s;
}


std::string COurTime::AsString() const
{
  char s[64];
  return ToString(s);
}


bool COurTime::IsLeapYear(int yy)
{
  return (yy % 4) == 0;  // true for 21'th century
}


const int* COurTime::GetMonthTable(int yy)
{
  static const int normal_year[12] = {0,31,59,90,120,151,181,212,243,273,304,334};
  static const int leap_year[12]   = {0,31,60,91,121,152,182,213,244,274,305,335};
  
  return IsLeapYear(yy) ? leap_year : normal_year;
}


////////////////////


bool IsStrEmpty(const char *s)
{
  return !s || !s[0];
}


std::string Base64Encode(const void *buff,unsigned len)
{
  std::string rc;

  const char *b64t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  const unsigned char *p = (const unsigned char*)buff;

  while ( len >= 3 )
  {
    unsigned t = 0;
    t |= (unsigned)(*p++) << 16; 
    t |= (unsigned)(*p++) << 8;  
    t |= (unsigned)(*p++);       

    rc += b64t[(t >> 18) & 0x3F];
    rc += b64t[(t >> 12) & 0x3F];
    rc += b64t[(t >> 6) & 0x3F];
    rc += b64t[t & 0x3F];

    len -= 3;
  }

  if ( len == 2 )
     {
       unsigned t = 0;
       t |= (unsigned)(*p++) << 16; 
       t |= (unsigned)(*p++) << 8;  

       rc += b64t[(t >> 18) & 0x3F];
       rc += b64t[(t >> 12) & 0x3F];
       rc += b64t[(t >> 6) & 0x3F];
       rc += '=';
     }
  else
  if ( len == 1 )
     {
       unsigned t = 0;
       t |= (unsigned)(*p++) << 16; 

       rc += b64t[(t >> 18) & 0x3F];
       rc += b64t[(t >> 12) & 0x3F];
       rc += '=';
       rc += '=';
     }

  return rc;
}


bool IsBoolEqu(bool b1,bool b2)
{
  return (b1 && b2) || (!b1 && !b2);
}



