
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_conf.h"
#include "stm32f4_discovery.h"
#include "waverecorder.h"
#include <stdio.h>
#include <stdlib.h>

//------------
static volatile int iTimingDelay = 0;

void msDelay(int ms)
{ 
  iTimingDelay = ms;
  while(iTimingDelay != 0);
}

void TimingDelay_Decrement(void)
{
  int t = iTimingDelay;
  t -= 10;
  t = t < 0 ? 0 : t;
  iTimingDelay = t;
}

//------------



#define AUDIO_FREQ  16000


int main(void)
{
  /* SysTick end of count event each 10ms */
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);

  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  STM_EVAL_LEDInit(LED5);
  STM_EVAL_LEDInit(LED6);

  /* Enable PLLI2S Clock */
  RCC_PLLI2SCmd(ENABLE);
  
  /* Wait till PLLI2S is ready */
  while(RCC_GetFlagStatus(RCC_FLAG_PLLI2SRDY) == RESET) 
  {
  }
  
  simple_rec_start(AUDIO_FREQ);
  
  while (1)
  {
  }
}




