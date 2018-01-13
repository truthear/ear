#include <stdio.h>
#include <string.h>
#include "board.h"
#include "radio.h"


#define SENDER



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





volatile bool sending = false;
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

bool high_power = true;  //!!!!!!!!!!!!


void EXTI9_5_IRQHandler()
{
  if ( EXTI_GetITStatus(EXTI_Line9) != RESET )
     {
       EXTI_ClearITPendingBit(EXTI_Line9);

       // button1

       LedOn(LedRed);
       DelayMs(1000);
       LedOff(LedRed);

       high_power = !high_power;
       reset_event = true;

     }
}



int main( void )
{
  //SystemCoreClockUpdate();  for discovery only

  DelayMs(1000);

  BoardInitMcu( );


  {
    GpioInit( &button, PD_9, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );

    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // enable sysclk
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,ENABLE);
    
    // Connect Button EXTI Line to GPIO Pin
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD,EXTI_PinSource9);

    // Configure Button EXTI line
    EXTI_InitStructure.EXTI_Line = EXTI_Line9;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // Enable and set Button EXTI Interrupt to the lowest priority
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure); 
  }

  while(1)
  {

  #ifdef SENDER  



    int packet_num = 0;
    
    while (1)
    {
      char Buffer[41];

      { // RFSolutions
        sprintf(Buffer,"RFSolutions %04d_______________________R",packet_num);

        SX1272Init();
        SX1272SetChannel( RF_FREQUENCY );
        SX1272SetTxConfig( LORA_BANDWIDTH, LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                           LORA_PREAMBLE_LENGTH, LORA_CRC_ON, LORA_IQ_INVERSION_ON );

        for ( int n = 0; n < 10; n++ )
            {
              LedOn(LedGreen);
              sending = true;
              Buffer[17] = '0'+n;
              SX1272Send( (uint8_t*)Buffer, sizeof(Buffer)-1 );
              while (sending);
              LedOff(LedGreen);
              DelayMs(1000);
            }

        SX1272SetSleep();
      }

      { // microchip
        sprintf(Buffer,"Microchip   %04d_______________________M",packet_num);

        Microchip_Init();

        for ( int n = 0; n < 10; n++ )
            {
              LedOn(LedOrange);
              Buffer[17] = '0'+n;
              Microchip_Send(Buffer);
              LedOff(LedOrange);
              DelayMs(1000);
            }
      }

      packet_num++;
    }
  #else
    SX1272Init();
      
    SX1272SetChannel( RF_FREQUENCY );
    
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
                printf("recv error!\n");
              }
           else
              {
                LedOn(LedBlue);
                printf("RSSI: %.1f, SNR: %.1f, [%d bytes]: ",recv_rssi,recv_snr,recv_size);
                for ( unsigned n = 0; n < recv_size; n++ )
                {
                  printf("%c",recv_data[n]);
                }
                printf("\n");
              }
           DelayMs(100);
           //LedOff(LedRed);
           LedOff(LedBlue);
         }
    }
  #endif
  }

}



