
#include "stm32f4xx.h"
#include "stm32f4_discovery.h"


#define FACTOR 100

volatile unsigned cnt = 0;


void SysTick_Handler(void)
{
  cnt++;

  if ( (cnt % FACTOR) == 0 )
     {
       STM_EVAL_LEDToggle(LED3);
       STM_EVAL_LEDToggle(LED4);
       STM_EVAL_LEDToggle(LED5);
       STM_EVAL_LEDToggle(LED6);
     }
}


int main()
{
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  STM_EVAL_LEDInit(LED5);
  STM_EVAL_LEDInit(LED6);

  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / FACTOR);

  while (1) 
  {
  }
}
