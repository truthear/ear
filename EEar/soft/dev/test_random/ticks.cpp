
#include "include.h"




void CTicksCommon::DelayInfinite()
{
  while (1) {}
}


///////////////


volatile unsigned CSysTicks::m_counter = 0;


void CSysTicks::Init(unsigned start_value)
{
  m_counter = start_value;
  
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency/1000);
}


unsigned CSysTicks::GetCounter()
{
  return m_counter;
}


void CSysTicks::Delay(unsigned ms)
{
  if ( ms )
     {
       if ( ms == INFINITE )
          {
            DelayInfinite();
          }
       else
          {
            unsigned t1 = GetCounter();
            while ( GetCounter() - t1 < ms ) {}
          }
     }
}


void CSysTicks::OnIRQ_Internal()
{
  m_counter++;  // assume operation is atomic on 32-bit CPU, overflow is ok
}


extern "C"
void SysTick_Handler(void)
{
  CSysTicks::OnIRQ_Internal();
}


/////////////////////



void CCPUTicks::Init()
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}


void CCPUTicks::Delay(unsigned ms)
{
  if ( ms )
     {
       if ( ms == INFINITE )
          {
            DelayInfinite();
          }
       else
          {
            unsigned max_seconds = (unsigned)0xFFFFFFFF/GetTicksPerSecond();  // truncate to second!
            unsigned max_ms = max_seconds*1000;

            ms = MIN(ms,max_ms);
            unsigned delta = ms*GetTicksPerMS();
               
            unsigned t1 = GetCounter();
            while ( GetCounter() - t1 < delta ) {}
          }
     }
}


unsigned CCPUTicks::GetTicksPerSecond()
{
  return ::SystemCoreClock;
}


unsigned CCPUTicks::GetTicksPerMS()
{
  return GetTicksPerSecond()/1000;
}


