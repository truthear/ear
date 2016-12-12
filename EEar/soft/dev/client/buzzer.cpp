
#include "include.h"


// global object:
CBuzzer g_buzzer(GPIOE,GPIO_Pin_9,RCC_AHB1Periph_GPIOE);



CBuzzer::CBuzzer(GPIO_TypeDef *_port,uint16_t _pin,uint32_t _clk)
   : m_pin(_port,_pin,_clk)
{
  m_pin.Off();

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
                   m_pin.Toggle();
                 }
            }

            m_pin.Off();
          }
     }
}



