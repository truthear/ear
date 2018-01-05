
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdio.h>
//#include <stdarg.h>
#include <stdlib.h>
#include <vector>
#include "dbg_uart.h"



void delay_ms_not_strict(uint32_t ms)
{
  volatile uint32_t nCount;
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  nCount = (RCC_Clocks.HCLK_Frequency / 10000) * ms;
  for(; nCount != 0; nCount --);
}


int v_data=1;
int v_bss;
std::vector<int> v;

void Test()
{
  volatile char st[1000];
  st[0] = 0;
  st[sizeof(st)-1] = 0;
  
  printf("data: %p, bss: %p, stack: %p, malloc: %p, %f, %lf\n",&v_data,&v_bss,st,malloc(1000),1.,1.);

  delay_ms_not_strict(1000);

  Test();
}


int main(void)
{
  InitUART2();

  //Test();
  
  while (1)
  {
    char c = GetChar();
    DbgChar(c);
  }
}

