
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
#include <stdio.h>
#include "dbg_uart.h"


#define BUTTON1_PIN                GPIO_Pin_9
#define BUTTON1_GPIO_PORT          GPIOD
#define BUTTON1_GPIO_CLK           RCC_AHB1Periph_GPIOD
#define BUTTON1_EXTI_LINE          EXTI_Line9
#define BUTTON1_EXTI_PORT_SOURCE   EXTI_PortSourceGPIOD
#define BUTTON1_EXTI_PIN_SOURCE    EXTI_PinSource9
#define BUTTON1_EXTI_IRQn          EXTI9_5_IRQn

#define BUTTON2_PIN                GPIO_Pin_10
#define BUTTON2_GPIO_PORT          GPIOD
#define BUTTON2_GPIO_CLK           RCC_AHB1Periph_GPIOD
#define BUTTON2_EXTI_LINE          EXTI_Line10
#define BUTTON2_EXTI_PORT_SOURCE   EXTI_PortSourceGPIOD
#define BUTTON2_EXTI_PIN_SOURCE    EXTI_PinSource10
#define BUTTON2_EXTI_IRQn          EXTI15_10_IRQn

#define BUTTON3_PIN                GPIO_Pin_11
#define BUTTON3_GPIO_PORT          GPIOD
#define BUTTON3_GPIO_CLK           RCC_AHB1Periph_GPIOD
#define BUTTON3_EXTI_LINE          EXTI_Line11
#define BUTTON3_EXTI_PORT_SOURCE   EXTI_PortSourceGPIOD
#define BUTTON3_EXTI_PIN_SOURCE    EXTI_PinSource11
#define BUTTON3_EXTI_IRQn          EXTI15_10_IRQn


static GPIO_TypeDef* BUTTON_PORT[]        = {BUTTON1_GPIO_PORT        ,BUTTON2_GPIO_PORT        ,BUTTON3_GPIO_PORT        }; 
static const uint16_t BUTTON_PIN[]        = {BUTTON1_PIN              ,BUTTON2_PIN              ,BUTTON3_PIN              }; 
static const uint32_t BUTTON_CLK[]        = {BUTTON1_GPIO_CLK         ,BUTTON2_GPIO_CLK         ,BUTTON3_GPIO_CLK         };
static const uint16_t BUTTON_EXTI_LINE[]  = {BUTTON1_EXTI_LINE        ,BUTTON2_EXTI_LINE        ,BUTTON3_EXTI_LINE        };
static const uint8_t BUTTON_PORT_SOURCE[] = {BUTTON1_EXTI_PORT_SOURCE ,BUTTON2_EXTI_PORT_SOURCE ,BUTTON3_EXTI_PORT_SOURCE };
static const uint8_t BUTTON_PIN_SOURCE[]  = {BUTTON1_EXTI_PIN_SOURCE  ,BUTTON2_EXTI_PIN_SOURCE  ,BUTTON3_EXTI_PIN_SOURCE  }; 
static const uint8_t BUTTON_IRQn[]        = {BUTTON1_EXTI_IRQn        ,BUTTON2_EXTI_IRQn        ,BUTTON3_EXTI_IRQn        };


void ButtonInit(int Button,int priority)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable the BUTTON Clock */
  RCC_AHB1PeriphClockCmd(BUTTON_CLK[Button], ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  /* Configure Button pin as input */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = BUTTON_PIN[Button];
  GPIO_Init(BUTTON_PORT[Button], &GPIO_InitStructure);

    /* Connect Button EXTI Line to Button GPIO Pin */
    SYSCFG_EXTILineConfig(BUTTON_PORT_SOURCE[Button], BUTTON_PIN_SOURCE[Button]);

    /* Configure Button EXTI line */
    EXTI_InitStructure.EXTI_Line = BUTTON_EXTI_LINE[Button];
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = BUTTON_IRQn[Button];
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = priority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = priority;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure); 
}




int main(void)
{
  InitUART3();
  
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  STM_EVAL_LEDInit(LED5);
  STM_EVAL_LEDInit(LED6);

  ButtonInit(0,10);
  ButtonInit(1,1);
  ButtonInit(2,1);
  
  while (1)
  {

  }
}


void delay_ms(uint32_t ms)
{
  volatile uint32_t nCount;
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  nCount = (RCC_Clocks.HCLK_Frequency / 10000) * ms;
  for(; nCount != 0; nCount --);
}


volatile int cnt = 0;

void EXTI9_5_IRQHandler()
{
  if ( EXTI_GetITStatus(BUTTON1_EXTI_LINE) != RESET )
  {
    EXTI_ClearITPendingBit(BUTTON1_EXTI_LINE);  

    STM_EVAL_LEDToggle(LED3);
//    delay_ms(100);
//    STM_EVAL_LEDOff(LED3);

    printf("%d\n",++cnt);
    
    delay_ms(2000);

  
  }
}


void EXTI15_10_IRQHandler()
{
  if ( EXTI_GetITStatus(BUTTON2_EXTI_LINE) != RESET )
  {
    cnt += 100;
    printf("%d\n",++cnt);
    
    //STM_EVAL_LEDOn(LED4);
    //delay_ms(100);
    //STM_EVAL_LEDOff(LED4);
  
    EXTI_ClearITPendingBit(BUTTON2_EXTI_LINE);  
  }
  
  if ( EXTI_GetITStatus(BUTTON3_EXTI_LINE) != RESET )
  {
    STM_EVAL_LEDOn(LED5);
    delay_ms(100);
    STM_EVAL_LEDOff(LED5);
  
    EXTI_ClearITPendingBit(BUTTON3_EXTI_LINE);  
  }
  
}

