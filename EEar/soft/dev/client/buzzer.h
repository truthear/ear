
#ifndef __BUZZER_H__
#define __BUZZER_H__


class CBuzzer
{
          CPin::EPins m_pin;
  
  public:
          // Warning!: CCPUTicks::Init() called inside!
          CBuzzer(CPin::EPins pin);

          // IRQ safe:
          void Activate(unsigned freq,unsigned duration);  // not more than ~25000 msec!
};


extern CBuzzer g_buzzer;


// like in windows, IRQ safe, not more than ~25000 msec!
#define Beep(freq,duration)  g_buzzer.Activate(freq,duration)


#endif
