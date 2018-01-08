/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Target board general functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include "board.h"





Gpio_t led1;
Gpio_t led2;
Gpio_t led3;
Gpio_t led4;


void BoardInitMcu( void )
{
  GpioInit(&led1,PD_12,PIN_OUTPUT,PIN_PUSH_PULL,PIN_PULL_UP,0);
  GpioInit(&led2,PD_13,PIN_OUTPUT,PIN_PUSH_PULL,PIN_PULL_UP,0);
  GpioInit(&led3,PD_14,PIN_OUTPUT,PIN_PUSH_PULL,PIN_PULL_UP,0);
  GpioInit(&led4,PD_15,PIN_OUTPUT,PIN_PUSH_PULL,PIN_PULL_UP,0);

  InitUART3();

}


void Delay( float s )
{
    DelayMs( s * 1000.0f );
}

void DelayMs( uint32_t ms )
{
  volatile uint32_t nCount;
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  nCount = (RCC_Clocks.HCLK_Frequency / 10000) * ms;
  for(; nCount != 0; nCount --);
}

