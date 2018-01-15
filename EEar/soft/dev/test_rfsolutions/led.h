
#ifndef __LED_H__
#define __LED_H__


#define BOARD_LED1         CPin::PD_12
#define BOARD_LED2         CPin::PD_13
#define BOARD_LED3         CPin::PD_14
#define BOARD_LED4         CPin::PD_15
#define BOARD_LED_GREEN    BOARD_LED1
#define BOARD_LED_ORANGE   BOARD_LED2
#define BOARD_LED_BLUE     BOARD_LED3
#define BOARD_LED_RED      BOARD_LED4



class CLED
{
          CPin::EPins m_pin;

  public:
          CLED(CPin::EPins led,bool init_on=false)
           : m_pin(led)
          {
            CPin::InitAsOutput(m_pin,init_on?1:0,GPIO_PuPd_UP);
          }

          void On()
          {
            CPin::Set(m_pin);
          }

          void Off()
          {
            CPin::Reset(m_pin);
          }

          void Toggle()
          {
            CPin::Toggle(m_pin);
          }

          void SetState(bool state)
          {
            if ( state )
               {
                 On();
               }
            else
               {
                 Off();
               }
          }
};



#endif
