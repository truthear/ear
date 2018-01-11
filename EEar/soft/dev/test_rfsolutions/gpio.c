/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Bleeper board GPIO driver implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include "board.h"
#include "gpio.h"


void GpioInit( Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config, PinTypes type, uint32_t value )
{
    GPIO_InitTypeDef GPIO_InitStructure;

    obj->pin = pin;

    if( pin == NC )
    {
        return;
    }

    obj->pinIndex = ( 0x01 << ( obj->pin & 0x0F ) );

    if( ( obj->pin & 0xF0 ) == 0x00 )
    {
        obj->port = GPIOA;
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    }
    else 
    if( ( obj->pin & 0xF0 ) == 0x10 )
    {
        obj->port = GPIOB;
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    }
    else 
    if( ( obj->pin & 0xF0 ) == 0x20 )
    {
        obj->port = GPIOC;
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    }
    else 
    if( ( obj->pin & 0xF0 ) == 0x30 )
    {
        obj->port = GPIOD;
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    }
    else
    if( ( obj->pin & 0xF0 ) == 0x40 )
    {
        obj->port = GPIOE;
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
    }
    else
    {
        //assert_param( FAIL );
    }

    GPIO_InitStructure.GPIO_Pin =  obj->pinIndex ;
    GPIO_InitStructure.GPIO_PuPd = obj->pull = type;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    if( mode == PIN_INPUT )
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    }
    else if( mode == PIN_ANALOGIC )
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    }
    else if( mode == PIN_ALTERNATE_FCT )
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    }
    else // mode output
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    }

    // Sets initial output value
    if( mode == PIN_OUTPUT )
    {
        GpioWrite( obj, value );
    }

    GPIO_Init( obj->port, &GPIO_InitStructure );

    // Sets initial output value
    if( mode == PIN_OUTPUT )
    {
        GpioWrite( obj, value );
    }

    if ( mode == PIN_ALTERNATE_FCT )
       {
         GPIO_PinAFConfig(obj->port, pin&0xF, value);
       }
}

void GpioWrite( Gpio_t *obj, uint32_t value )
{
    // Check if pin is not connected
    if( obj->pin == NC )
    {
        return;
    }
    if ( value )
     GPIO_SetBits(obj->port,obj->pinIndex);
    else
     GPIO_ResetBits(obj->port,obj->pinIndex);


}

void GpioToggle( Gpio_t *obj )
{
    // Check if pin is not connected
    if( obj->pin == NC )
    {
        return;
    }
    GPIO_ToggleBits( obj->port, obj->pinIndex );
}

uint32_t GpioRead( Gpio_t *obj )
{
    // Check if pin is not connected
    if( obj->pin == NC )
    {
        return 0;
    }
    return GPIO_ReadInputDataBit( obj->port, obj->pinIndex );
}


