
/* Includes ------------------------------------------------------------------*/
#include "include.h"


int main(void)
{
  //CCPUTicks::Init();
  CSysTicks::Init();
  InitUART3();

  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG,ENABLE);
  RNG_Cmd(ENABLE);

  while (1)
  {
    while ( RNG_GetFlagStatus(RNG_FLAG_DRDY) == RESET ) {}

    unsigned rnd = RNG_GetRandomNumber();

    printf("random: %08X\n",rnd);

    Sleep(1000);
  }
}

