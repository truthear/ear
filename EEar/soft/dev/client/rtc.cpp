
#include "include.h"



volatile OURTIME CRTC::m_shift = 0;



bool CRTC::Init(char yy,char mm,char dd,char wd,char hh,char nn,char ss)
{
  bool rc = false;

  // Enable the PWR clock
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

  // Allow access to RTC
  PWR_BackupAccessCmd(ENABLE);

  // Reset registers in backup domain
  RCC_BackupResetCmd(ENABLE);
  RCC_BackupResetCmd(DISABLE);
  
  // Init LSE
  RCC_LSEConfig(RCC_LSE_ON);
  while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {}
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

  // Enable the RTC Clock
  RCC_RTCCLKCmd(ENABLE);
  
  // Wait for RTC APB registers synchronisation
  RTC_WaitForSynchro();

  RTC_InitTypeDef RTC_InitStruct;
  RTC_InitStruct.RTC_AsynchPrediv = (1UL << (15-SUBSECONDS_BITS)) - 1;
  RTC_InitStruct.RTC_SynchPrediv  = (1UL << SUBSECONDS_BITS) - 1;
  RTC_InitStruct.RTC_HourFormat = RTC_HourFormat_24;

  if ( RTC_Init(&RTC_InitStruct) == SUCCESS )
     {
       // set date
       RTC_DateTypeDef RTC_DateStruct;
       RTC_DateStruct.RTC_Year = yy;
       RTC_DateStruct.RTC_Month = mm;
       RTC_DateStruct.RTC_Date = dd;
       RTC_DateStruct.RTC_WeekDay = wd;
       RTC_SetDate(RTC_Format_BIN, &RTC_DateStruct);

       // set time
       RTC_TimeTypeDef RTC_TimeStruct;
       RTC_TimeStruct.RTC_Hours = hh;
       RTC_TimeStruct.RTC_Minutes = nn;
       RTC_TimeStruct.RTC_Seconds = ss;
       RTC_TimeStruct.RTC_H12 = 0;
       RTC_SetTime(RTC_Format_BIN, &RTC_TimeStruct);
       
       // init timestamp
       RTC_TimeStampPinSelection(RTC_TimeStampPin_PC13);
       RTC_ClearFlag(RTC_FLAG_TSF|RTC_FLAG_TSOVF);  // clear TS and TS_Overflow flags
       RTC_TimeStampCmd(RTC_TimeStampEdge_Rising,ENABLE);

       rc = true;
     }

  return rc;
}


void CRTC::SetShift(OURTIME shift)
{
  m_shift = shift;
}


OURTIME CRTC::GetTime(bool use_shift)
{
  char yy,mm,dd,wd,hh,nn,ss;
  int subsec;

  GetTime(yy,mm,dd,wd,hh,nn,ss,subsec);

  OURTIME t = ConvertOurTime(2000+yy,mm,dd,hh,nn,ss,SS2MS(subsec));

  return use_shift ? t+m_shift : t;
}


bool CRTC::GetTS(OURTIME& _time,bool use_shift)
{
  char yy,mm,dd,wd,hh,nn,ss;
  int subsec;

  if ( GetTS(yy,mm,dd,wd,hh,nn,ss,subsec) )
     {
       OURTIME t = ConvertOurTime(2000+yy,mm,dd,hh,nn,ss,SS2MS(subsec));

       _time = use_shift ? t+m_shift : t;

       return true;
     }
  else
     {
       return false;
     }
}


void CRTC::GetTime(char& yy,char& mm,char& dd,char& wd,char& hh,char& nn,char& ss,int& subsec)
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


bool CRTC::GetTS(char& yy,char& mm,char& dd,char& wd,char& hh,char& nn,char& ss,int& subsec)
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


int CRTC::SS2MS(int subs)
{
  int mx = (1L << SUBSECONDS_BITS) - 1;

  return mx < subs ? 0 : (((mx-subs) * 1000) >> SUBSECONDS_BITS);
}






