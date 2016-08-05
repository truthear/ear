
#ifndef __TICKS_H__
#define __TICKS_H__


class CTicksCommon
{
  public:
          static const unsigned INFINITE = 0xFFFFFFFF;

          static void DelayInfinite();
};


// WARNING!!! This class uses IRQ, so Delay() cannot be called inside another IRQ handler - 
//            it will cause INFINITE LOOP!!!
//
// Counter incremented each 1 msec
class CSysTicks : public CTicksCommon
{
          static volatile unsigned m_counter;

  public:
          static void Init(unsigned start_value=0);
          static unsigned GetCounter();    // msec
          static void Delay(unsigned ms);  // do not use inside IRQ handler!!!

  public:
          // used internally:
          static void OnIRQ_Internal();
};


// WARNING!!! Overflow of counter is about ~25 sec, so it cannot be used in long delays
// Counter incremented each 1 CPU tick
class CCPUTicks : public CTicksCommon
{
  public:
          static void Init();
          static unsigned GetCounter() { return DWT->CYCCNT; }    // in CPU ticks
          static void Delay(unsigned ms);  // max about ~25000 msec
          static unsigned GetTicksPerSecond();
          static unsigned GetTicksPerMS();
};



#endif

