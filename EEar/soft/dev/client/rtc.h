
#ifndef __RTC_H__
#define __RTC_H__


class CRTC
{
          static const int SUBSECONDS_BITS = 10;  // to get 1/1024 sec resolution (min is 8 bits, max is 15)

  public:
          static bool Init();

          static int GetSSBitsWide() { return SUBSECONDS_BITS; }
          static int SS2MS(int subs);

          static void SetTime(char yy,char mm,char dd,char wd,char hh,char nn,char ss);
          static void GetTime(char& yy,char& mm,char& dd,char& wd,char& hh,char& nn,char& ss,int& subsec);

          // this function also clears TS and TS_Overflow flags:
          static bool GetTS(char& yy,char& mm,char& dd,char& wd,char& hh,char& nn,char& ss,int& subsec);

  private:
          enum ERtcClk {
          RTC_CLKSEL_INTERNAL = 0,
          RTC_CLKSEL_EXTERNAL = 1,
          };

          static bool InitRTC(ERtcClk clk,int subseconds_bits);
          static void InitTimeStamp(uint32_t ts_pin,uint32_t ts_edge);

};


#endif
