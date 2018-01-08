#include <stdio.h>
#include <string.h>
#include "board.h"
#include "radio.h"


//#define SENDER



#define RF_FREQUENCY                                868000000 // Hz
#define LORA_BANDWIDTH                              2         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_CRC_ON                                 true
#define LORA_IQ_INVERSION_ON                        false

#define LORA_SYMBOL_TIMEOUT                         5         // Symbols, for RX only




volatile bool sending = false;
volatile bool send_err = false;
volatile bool receiving = false;
volatile bool recv_packet = false;
volatile bool recv_err = false;

uint16_t recv_size = 0;
float recv_rssi = 0;
float recv_snr = 0;
uint8_t recv_data[256];


void OnTxDone()
{
    sending = false;
    send_err = false;
    //SX1272Sleep();
}


void OnRxDone( const uint8_t *payload, uint16_t size, float rssi, float snr )
{
    receiving = false;
    recv_err = false;
    recv_packet = true;
    recv_size = size;
    recv_rssi = rssi;
    recv_snr = snr;
    memcpy(recv_data,payload,size);
}


void OnRxError( void )
{
    receiving = false;
    recv_err = true;
    recv_packet = false;
}

Gpio_t button;
volatile bool reset_event = false;

void EXTI0_IRQHandler()
{
  if ( EXTI_GetITStatus(EXTI_Line0) != RESET )
     {
       EXTI_ClearITPendingBit(EXTI_Line0);

       LedOn(LedRed);
       DelayMs(1000);
       LedOff(LedRed);
       reset_event = true;
     }
}



int main( void )
{
  SystemCoreClockUpdate();

  DelayMs(1000);

  BoardInitMcu( );

  {
    GpioInit( &button, RADIO_DIO_0, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );

    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // enable sysclk
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,ENABLE);
    
    // Connect Button EXTI Line to GPIO Pin
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA,EXTI_PinSource0);

    // Configure Button EXTI line
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // Enable and set Button EXTI Interrupt to the lowest priority
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure); 

  }

  while(1)
  {
    SX1272Init();

    LedOn(LedOrange);
    int version = SX1272GetVersion();
    printf("version: %d\n",version);
    LedOff(LedOrange);

      
    SX1272SetChannel( RF_FREQUENCY );

  #ifdef SENDER  
    SX1272SetTxConfig( LORA_BANDWIDTH, LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                       LORA_PREAMBLE_LENGTH, LORA_CRC_ON, LORA_IQ_INVERSION_ON );

    uint8_t Buffer[40] = {'A',};

    printf("TimeOnAir: %d msec\n",(int)SX1272GetTimeOnAir(sizeof(Buffer)));
    
    while (1)
    {
      LedOn(LedGreen);
      LedOff(LedRed);
      sending = true;
      SX1272Send( Buffer, sizeof(Buffer) );
      while (sending&&!reset_event);
      if ( reset_event )
         {
           reset_event = false;
           break;
         }

      LedOff(LedGreen);
      if ( send_err )
       LedOn(LedRed);

      DelayMs(1000);
    }
  #else
    SX1272SetRxConfig( LORA_BANDWIDTH, LORA_SPREADING_FACTOR, LORA_CODINGRATE, 
                       LORA_PREAMBLE_LENGTH, LORA_SYMBOL_TIMEOUT, LORA_CRC_ON, LORA_IQ_INVERSION_ON );

    SX1272SetRx();
    while (1)
    {
      receiving = true;
      recv_err = false;
      recv_packet = false;
      while(receiving&&!reset_event);
      if ( reset_event )
         {
           reset_event = false;
           break;
         }
      if ( recv_packet || recv_err )
         {
           if ( recv_err )
              {
                LedOn(LedRed);
              }
           else
              {
                LedOn(LedBlue);
                printf("RSSI: %.1f, SNR: %.1f, [%d bytes]\n",recv_rssi,recv_snr,recv_size);
                for ( unsigned n = 0; n < recv_size; n++ )
                {
                  printf("%02X",recv_data[n]);
                }
                printf("\n");
              }
           DelayMs(300);
           LedOff(LedRed);
           LedOff(LedBlue);
         }
    }
  #endif
  }

}



