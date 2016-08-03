
#ifndef __RTC_H__
#define __RTC_H__


class CRTC
{
          static const int SUBSECONDS_BITS = 10;  // to get 1/1024 sec resolution (min is 8 bits, max is 15)

          static volatile OURTIME m_shift;

  public:
          static bool Init(char yy,char mm,char dd,char wd,char hh,char nn,char ss);

          static void SetShift(OURTIME shift);

          static OURTIME GetTime(bool use_shift=true);
          static bool GetTS(OURTIME& _time,bool use_shift=false);

  private:
          static void GetTime(char& yy,char& mm,char& dd,char& wd,char& hh,char& nn,char& ss,int& subsec);
          static bool GetTS(char& yy,char& mm,char& dd,char& wd,char& hh,char& nn,char& ss,int& subsec);
          static int SS2MS(int subs);

};


#endif
