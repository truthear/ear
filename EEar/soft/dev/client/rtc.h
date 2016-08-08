
#ifndef __RTC_H__
#define __RTC_H__


class CRTC
{
  public:
          typedef void (*TCALLBACK)(void*,OURTIME ts_unshifted,OURTIME ts_shifted);

  private:        
          static const int SUBSECONDS_BITS = 10;  // to get 1/1024 sec resolution (min is 8 bits, max is 15)

          static volatile OURTIME m_shift;

          static TCALLBACK p_cb;
          static void *p_cbparm;

  public:
          static bool Init(char yy,char mm,char dd,char wd,char hh,char nn,char ss,
                           TCALLBACK cbts=NULL,void *cbparm=NULL,int irq_priority=2);

          static void SetShift(OURTIME shift);
          static OURTIME GetShift(); // for debug

          static OURTIME GetTime(); // WARNING!!! it waits for 1/32768 sec, not thread-safe! returns shifted time
          
          static bool GetTS(OURTIME& _time,bool use_shift=false);  // use it for polling mode only (non-interrupt!)

  private:
          static void GetTime(char& yy,char& mm,char& dd,char& wd,char& hh,char& nn,char& ss,int& subsec);
          static bool GetTS(char& yy,char& mm,char& dd,char& wd,char& hh,char& nn,char& ss,int& subsec);
          static int SS2MS(int subs);

  public:
          // used internally:
          static void OnIRQ_Internal();

};


#endif
