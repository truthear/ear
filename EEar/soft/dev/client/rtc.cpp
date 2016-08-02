
#include "include.h"



bool CBoardRTC::InitRTC(ERtcClk clk,int subseconds_bits)
{
  // Enable the PWR clock
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

  // Allow access to RTC
  PWR_BackupAccessCmd(ENABLE);

  // Reset registers in backup domain
  RCC_BackupResetCmd(ENABLE);
  RCC_BackupResetCmd(DISABLE);
  
  if ( clk == RTC_CLKSEL_INTERNAL )
     {
       RCC_LSICmd(ENABLE);
       while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {}
       RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
     }
  else
     {
       RCC_LSEConfig(RCC_LSE_ON);
       while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {}
       RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
     }

  // Enable the RTC Clock
  RCC_RTCCLKCmd(ENABLE);
  
  // Wait for RTC APB registers synchronisation
  RTC_WaitForSynchro();

  RTC_InitTypeDef RTC_InitStruct;
  RTC_InitStruct.RTC_AsynchPrediv = (1UL << (15-subseconds_bits)) - 1;
  RTC_InitStruct.RTC_SynchPrediv  = (1UL << subseconds_bits) - 1;
  RTC_InitStruct.RTC_HourFormat = RTC_HourFormat_24;
  return (RTC_Init(&RTC_InitStruct) == SUCCESS);
}


void CBoardRTC::InitTimeStamp(uint32_t ts_pin,uint32_t ts_edge)
{
  RTC_TimeStampPinSelection(ts_pin);
  RTC_ClearFlag(RTC_FLAG_TSF|RTC_FLAG_TSOVF);  // clear TS and TS_Overflow flags
  RTC_TimeStampCmd(ts_edge,ENABLE);
}


bool CBoardRTC::Init()
{
  if ( !InitRTC(RTC_CLKSEL_EXTERNAL/*we use LSE*/,SUBSECONDS_BITS) )
     {
       return false;
     }
  else
     {
       InitTimeStamp(RTC_TimeStampPin_PC13,RTC_TimeStampEdge_Rising);
       return true;
     }
}


void CBoardRTC::SetTime(char yy,char mm,char dd,char wd,char hh,char nn,char ss)
{
  RTC_DateTypeDef RTC_DateStruct;
  RTC_DateStruct.RTC_Year = yy;
  RTC_DateStruct.RTC_Month = mm;
  RTC_DateStruct.RTC_Date = dd;
  RTC_DateStruct.RTC_WeekDay = wd;
  RTC_SetDate(RTC_Format_BIN, &RTC_DateStruct);

  RTC_TimeTypeDef RTC_TimeStruct;
  RTC_TimeStruct.RTC_Hours = hh;
  RTC_TimeStruct.RTC_Minutes = nn;
  RTC_TimeStruct.RTC_Seconds = ss;
  RTC_TimeStruct.RTC_H12 = 0;
  RTC_SetTime(RTC_Format_BIN, &RTC_TimeStruct);
}


void CBoardRTC::GetTime(char& yy,char& mm,char& dd,char& wd,char& hh,char& nn,char& ss,int& subsec)
{
  subsec = ((uint32_t)(RTC->SSR)) & 0xFFFF;  // after read SSR time/date regs became frozen

  RTC_TimeTypeDef t_s;
  RTC_GetTime(RTC_Format_BIN,&t_s);
  hh = t_s.RTC_Hours;
  nn = t_s.RTC_Minutes;
  ss = t_s.RTC_Seconds;

  RTC_DateTypeDef d_s;
  RTC_GetDate(RTC_Format_BIN,&d_s);  // regs are ufrozen now!
  yy = d_s.RTC_Year;
  mm = d_s.RTC_Month;
  dd = d_s.RTC_Date;
  wd = d_s.RTC_WeekDay;
}


bool CBoardRTC::GetTS(char& yy,char& mm,char& dd,char& wd,char& hh,char& nn,char& ss,int& subsec)
{
  if ( RTC_GetFlagStatus(RTC_FLAG_TSF) != RESET )
     {
       subsec = (uint32_t)(RTC->TSSSR) & 0xFFFF;

       RTC_TimeTypeDef t_s;
       RTC_DateTypeDef d_s;
       RTC_GetTimeStamp(RTC_Format_BIN,&t_s,&d_s);  // here year returned is 0

       RTC_DateTypeDef year_s;
       RTC_GetDate(RTC_Format_BIN,&year_s);  // get year from regular time counter

       yy = year_s.RTC_Year;
       mm = d_s.RTC_Month;
       dd = d_s.RTC_Date;
       wd = d_s.RTC_WeekDay;
       hh = t_s.RTC_Hours;
       nn = t_s.RTC_Minutes;
       ss = t_s.RTC_Seconds;

       RTC_ClearFlag(RTC_FLAG_TSF|RTC_FLAG_TSOVF);

       return true;
     }
  else
     {
       return false;
     }
}


int CBoardRTC::SS2MS(int subs)
{
  int mx = (1L << SUBSECONDS_BITS) - 1;

  return ((mx-subs) * 1000) >> SUBSECONDS_BITS;
}






