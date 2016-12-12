
#include "include.h"



CButton::CButton(GPIO_TypeDef *_port,uint16_t _pin,uint32_t _clk)
{
  p_port = _port;
  m_pin = _pin;

  InitGPIO_Internal(_port,_pin,_clk);
}


CButton::CButton(GPIO_TypeDef *_port,uint16_t _pin,uint32_t _clk,
                 uint16_t _exti_line,uint8_t _port_source,uint8_t _pin_source,uint8_t _irqn)
{
  p_port = _port;
  m_pin = _pin;

  InitGPIO_Internal(_port,_pin,_clk);

  // enable sysclk
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,ENABLE);
  
  // Connect Button EXTI Line to Button GPIO Pin
  SYSCFG_EXTILineConfig(_port_source,_pin_source);

  // Configure Button EXTI line
  EXTI_InitTypeDef EXTI_InitStructure;
  EXTI_InitStructure.EXTI_Line = _exti_line;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  // Enable and set Button EXTI Interrupt to the lowest priority
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = _irqn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); 
}


CButton::~CButton()
{
}


bool CButton::IsDown()
{
  return (GPIO_ReadInputDataBit(p_port,m_pin) == 0);
}


void CButton::InitGPIO_Internal(GPIO_TypeDef *port,uint16_t pin,uint32_t clk)
{
  RCC_AHB1PeriphClockCmd(clk,ENABLE);

  GPIO_InitTypeDef i;
  i.GPIO_Mode = GPIO_Mode_IN;
  i.GPIO_PuPd = GPIO_PuPd_UP;
  i.GPIO_Pin = pin;
  GPIO_Init(port,&i);
}




static GPIO_TypeDef* BUTTON_PORT[]        = {GPIOD                ,GPIOD                ,GPIOD                }; 
static const uint16_t BUTTON_PIN[]        = {GPIO_Pin_9           ,GPIO_Pin_10          ,GPIO_Pin_11          }; 
static const uint32_t BUTTON_CLK[]        = {RCC_AHB1Periph_GPIOD ,RCC_AHB1Periph_GPIOD ,RCC_AHB1Periph_GPIOD };
static const uint16_t BUTTON_EXTI_LINE[]  = {EXTI_Line9           ,EXTI_Line10          ,EXTI_Line11          };
static const uint8_t BUTTON_PORT_SOURCE[] = {EXTI_PortSourceGPIOD ,EXTI_PortSourceGPIOD ,EXTI_PortSourceGPIOD };
static const uint8_t BUTTON_PIN_SOURCE[]  = {EXTI_PinSource9      ,EXTI_PinSource10     ,EXTI_PinSource11     }; 
static const uint8_t BUTTON_IRQn[]        = {EXTI9_5_IRQn         ,EXTI15_10_IRQn       ,EXTI15_10_IRQn       };

                                                                                                                 
CBoardButton::TCALLBACKWITHPARM CBoardButton::callbacks[3] = {{NULL,NULL},{NULL,NULL},{NULL,NULL}};
                                                                                                                 
                                                                                                                 
CBoardButton::CBoardButton(EBoardButtons btn)
 : CButton(BUTTON_PORT[btn],BUTTON_PIN[btn],BUTTON_CLK[btn])
{
  callbacks[btn].func = NULL;
}
                                                                                                                 
                                                                                                                 
CBoardButton::CBoardButton(EBoardButtons btn,TCALLBACK func,void *cbparm)
 : CButton(BUTTON_PORT[btn],BUTTON_PIN[btn],BUTTON_CLK[btn],BUTTON_EXTI_LINE[btn],BUTTON_PORT_SOURCE[btn],BUTTON_PIN_SOURCE[btn],BUTTON_IRQn[btn])
{
  callbacks[btn].parm = cbparm;  // parm should be set first!
  callbacks[btn].func = func;
}


void CBoardButton::OnIRQ_Internal(EBoardButtons btn)
{
  uint16_t line = BUTTON_EXTI_LINE[btn];
  
  if ( EXTI_GetITStatus(line) != RESET )
     {
       EXTI_ClearITPendingBit(line);

       if ( callbacks[btn].func )
          {
            callbacks[btn].func(callbacks[btn].parm);
          }
     }
}
                                                                                                                 
                                                                                                                 
extern "C"
void EXTI9_5_IRQHandler()
{
  CBoardButton::OnIRQ_Internal(BOARD_BUTTON1);
}


extern "C"
void EXTI15_10_IRQHandler()
{
  CBoardButton::OnIRQ_Internal(BOARD_BUTTON2);
  CBoardButton::OnIRQ_Internal(BOARD_BUTTON3);
}





