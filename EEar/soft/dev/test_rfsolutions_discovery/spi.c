/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Bleeper board SPI driver implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include "board.h"


void SpiFrequency( Spi_t *obj, uint32_t hz )
{
    uint32_t divisor = 0;
    uint32_t sysClkTmp = SystemCoreClock;
    uint32_t baudRate;

    while( sysClkTmp > hz )
    {
        divisor++;
        sysClkTmp = ( sysClkTmp >> 1 );

        if( divisor >= 7 )
        {
            break;
        }
    }

    baudRate =( ( ( divisor & 0x4 ) == 0 ) ? 0x0 : SPI_CR1_BR_2 ) |
              ( ( ( divisor & 0x2 ) == 0 ) ? 0x0 : SPI_CR1_BR_1 ) |
              ( ( ( divisor & 0x1 ) == 0 ) ? 0x0 : SPI_CR1_BR_0 );

    //printf("SPI baudRate: %d (0x%X), SystemCoreClock: %d\n",baudRate,baudRate,SystemCoreClock);
    // discovery board: SPI_BaudRatePrescaler_16
    // ear board:       SPI_BaudRatePrescaler_64
    
    obj->Spi.SPI_BaudRatePrescaler = baudRate;
}


void SpiInit( Spi_t *obj, PinNames mosi, PinNames miso, PinNames sclk, PinNames nss )
{
  obj->inst = SPI1;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

  SPI_Cmd(obj->inst, DISABLE);
  SPI_I2S_DeInit(obj->inst);
  
  GpioInit( &obj->Mosi, mosi, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_DOWN, GPIO_AF_SPI1 );
  GpioInit( &obj->Miso, miso, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_DOWN, GPIO_AF_SPI1 );
  GpioInit( &obj->Sclk, sclk, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_DOWN, GPIO_AF_SPI1 );
  GpioInit( &obj->Nss, nss, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );

  obj->Spi.SPI_Mode = SPI_Mode_Master;
  obj->Spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  obj->Spi.SPI_DataSize = SPI_DataSize_8b;
  obj->Spi.SPI_CPOL = SPI_CPOL_Low;
  obj->Spi.SPI_CPHA = SPI_CPHA_1Edge;
  obj->Spi.SPI_FirstBit = SPI_FirstBit_MSB;
  obj->Spi.SPI_NSS = SPI_NSS_Soft;
  obj->Spi.SPI_CRCPolynomial = 7;

  //obj->Spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;  //baudRate;
  SpiFrequency( obj, 10000000 );

  SPI_Init(obj->inst, &obj->Spi );
  SPI_Cmd(obj->inst, ENABLE);
}



uint16_t SpiInOut( Spi_t *obj, uint16_t outData )
{
    while( SPI_I2S_GetFlagStatus( obj->inst, SPI_I2S_FLAG_TXE ) == RESET );
    SPI_I2S_SendData(obj->inst,outData & 0xFF);

    while( SPI_I2S_GetFlagStatus( obj->inst, SPI_I2S_FLAG_RXNE ) == RESET );
    uint16_t rc = SPI_I2S_ReceiveData(obj->inst);

    return rc;
}

