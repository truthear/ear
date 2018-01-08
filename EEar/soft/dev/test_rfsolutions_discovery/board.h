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
#ifndef __BOARD_H__
#define __BOARD_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_syscfg.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_spi.h"
#include "misc.h"
#include "gpio.h"
#include "spi.h"
#include "radio.h"
#include "dbg_uart.h"


#define RADIO_MOSI                                  PA_7   //SPI1, do not change!
#define RADIO_MISO                                  PA_6   //SPI1, do not change!
#define RADIO_SCLK                                  PA_5   //SPI1, do not change!
#define RADIO_NSS                                   PA_4   //SPI1, do not change!
#define RADIO_RESET                                 PE_2
#define RADIO_ANT_SWITCH_RX                         PE_4
#define RADIO_ANT_SWITCH_TX                         PE_3
#define RADIO_DIO_0                                 PA_3


extern Gpio_t led1;
extern Gpio_t led2;
extern Gpio_t led3;
extern Gpio_t led4;

#define LedGreen  led1
#define LedOrange led2
#define LedBlue   led4
#define LedRed    led3

#define LedOn(l)   {  GpioWrite(&l,1); }
#define LedOff(l)   {   GpioWrite(&l,0); }
#define LedToggle(l)   {   GpioToggle(&l); }



/*!
 * \brief Initializes the target board peripherals.
 */
void BoardInitMcu( void );


/*! 
 * Blocking delay of "s" seconds
 */
void Delay( float s );

/*! 
 * Blocking delay of "ms" milliseconds
 */
void DelayMs( uint32_t ms );


void OnTxDone();
void OnRxDone( const uint8_t *payload, uint16_t size, float rssi, float snr );
void OnRxError();



#endif // __BOARD_H__
