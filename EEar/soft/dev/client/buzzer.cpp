
#include "include.h"


// global object:
CBuzzer g_buzzer(CPin::PE_9);



CBuzzer::CBuzzer(CPin::EPins pin)
   : m_pin(pin)
{
  CPin::InitAsOutput(m_pin,0);

  CCPUTicks::Init();
}


void CBuzzer::Activate(unsigned freq,unsigned duration)
{
  if ( duration > 0 )
     {
       if ( freq < 1 || freq > 20000 )
          {
            CCPUTicks::Delay(duration);
          }
       else
          {
            unsigned numticks = duration * CCPUTicks::GetTicksPerMS();  // overflow possible!

            unsigned freq_delta_ticks = CCPUTicks::GetTicksPerSecond() / freq;
            
            unsigned starttick = CCPUTicks::GetCounter();
            unsigned old_cycle = 0xFFFFFFFF;
            
            while ( 1 )
            {
              unsigned curr_ticks = CCPUTicks::GetCounter();
              
              if ( duration != CTicksCommon::INFINITE )
                 {
                   if ( curr_ticks - starttick > numticks )
                     break;
                 }

              unsigned cycle = (curr_ticks - starttick) / freq_delta_ticks;
              if ( cycle != old_cycle )
                 {
                   old_cycle = cycle;
                   CPin::Toggle(m_pin);
                 }
            }

            CPin::Reset(m_pin);
          }
     }
}



