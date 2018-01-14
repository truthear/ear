
#ifndef __BUTTON_H__
#define __BUTTON_H__


#define BOARD_BUTTON1 CPin::PD_9
#define BOARD_BUTTON2 CPin::PD_10
#define BOARD_BUTTON3 CPin::PD_11


class CButton
{
          CPin::EPins m_pin;
  
  public:
          CButton(CPin::EPins btn,CPin::TCALLBACK func=NULL,void *cbparm=NULL) 
             : m_pin(btn)
          {
            CPin::InitAsInput(m_pin,GPIO_PuPd_UP);

            if ( func )
               {
                 CPin::SetInterrupt(m_pin,func,cbparm,EXTI_Trigger_Falling);
               }
          }

          bool IsDown() const
          {
            return CPin::GetValue(m_pin) == 0;
          }
};


#endif


