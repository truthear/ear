
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdio.h>
#include <stdarg.h>
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

  
  
  while (1)
  {
    double f = 0;
    sscanf("111.222","%lf",&f);
    printf("%08X %08X %lf\n",*(reinterpret_cast<unsigned*>(&f)+0),*(reinterpret_cast<unsigned*>(&f)+1),f);

    delay_ms_not_strict(1000);
  }

}

