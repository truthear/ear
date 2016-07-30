
#ifndef __LED_H__
#define __LED_H__



class CLED
{
          GPIO_TypeDef *p_port;
          uint16_t m_pin;

  public:
          CLED(GPIO_TypeDef *_port,uint16_t _pin,uint32_t _clk);
          virtual ~CLED();

          void On();
          void Off();
          void Toggle();
};


enum EBoardLeds {
BOARD_LED1      = 0,
BOARD_LED2      = 1,
BOARD_LED3      = 2,
BOARD_LED4      = 3,
BOARD_LED_GREEN   = BOARD_LED1,
BOARD_LED_ORANGE  = BOARD_LED2,
BOARD_LED_BLUE    = BOARD_LED3,
BOARD_LED_RED     = BOARD_LED4,
};

class CBoardLED : public CLED
{
          static uint16_t leds[4];
  public:
          CBoardLED(EBoardLeds led,bool init_on=false);
};



#endif
