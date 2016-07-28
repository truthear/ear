
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdio.h>
//#include <stdarg.h>
//#include <stdlib.h>
//#include <vector>
#include "dbg_uart.h"



void delay_ms_not_strict(uint32_t ms)
{
  volatile uint32_t nCount;
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  nCount = (RCC_Clocks.HCLK_Frequency / 10000) * ms;
  for(; nCount != 0; nCount --);
}



int main(void)
{
  InitUART3();

  RCC_ClocksTypeDef clk;
  RCC_GetClocksFreq(&clk);
  
  while (1)
  {
    printf("%u %u %u %u\n",(unsigned)clk.SYSCLK_Frequency,(unsigned)clk.HCLK_Frequency,(unsigned)clk.PCLK1_Frequency,(unsigned)clk.PCLK2_Frequency);

    delay_ms_not_strict(1000);
  }

}

