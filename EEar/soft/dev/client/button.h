
#ifndef __BUTTON_H__
#define __BUTTON_H__


class CButton
{
          GPIO_TypeDef *p_port;
          uint16_t m_pin;

  public:
          CButton(GPIO_TypeDef *_port,uint16_t _pin,uint32_t _clk);
          CButton(GPIO_TypeDef *_port,uint16_t _pin,uint32_t _clk,uint16_t _exti_line,uint8_t _port_source,uint8_t _pin_source,uint8_t _irqn);
          virtual ~CButton();

          bool IsDown();

  private:
          static void InitGPIO_Internal(GPIO_TypeDef *port,uint16_t pin,uint32_t clk);
};


enum EBoardButtons {
BOARD_BUTTON1 = 0,
BOARD_BUTTON2 = 1,
BOARD_BUTTON3 = 2,
};

class CBoardButton : public CButton
{
  public:
          typedef void (*TCALLBACK)(void*);

  private:        
          typedef struct {
          TCALLBACK func;
          void *parm;
          } TCALLBACKWITHPARM;

          static TCALLBACKWITHPARM callbacks[3];

  public:
          CBoardButton(EBoardButtons btn);
          CBoardButton(EBoardButtons btn,TCALLBACK func,void *cbparm=NULL);

  public:
          // used internally:
          static void OnIRQ_Internal(EBoardButtons btn);
};


#endif


