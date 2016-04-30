
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_conf.h"
#include "stm32f4_discovery.h"
#include <stdio.h>


//         RX   TX
// ------------------------
// USART2:  PD6  PD5  - GSM
// USART3:  PB11 PB10 - debug !
// ------------------------




#define MAX_BUFF 4096

typedef struct {
unsigned char buff[MAX_BUFF];
unsigned to_write;
unsigned to_read;
} TBUFF;

volatile TBUFF debug2device;
volatile TBUFF device2debug;

void InitTBuff(volatile TBUFF *i)
{
  i->to_write = 0;
  i->to_read = 0;
}


void delay_ms_not_strict(uint32_t ms);
void msDelay(int ms);


unsigned SafeIncrement(unsigned idx)
{
  volatile unsigned t = idx;
  t++;
  if ( t == MAX_BUFF )
   t = 0;

  return t;
}


void BuffWrite(volatile TBUFF *i,unsigned char c)
{
  volatile unsigned idx = i->to_write;
  i->buff[idx] = c;
  i->to_write = SafeIncrement(idx);
}


// debug receive
void USART3_IRQHandler()
{
  if ( USART_GetITStatus(USART3,USART_IT_RXNE) != RESET )
     {
       uint16_t c = USART_ReceiveData(USART3);
       USART_ClearITPendingBit(USART3,USART_IT_RXNE);

       BuffWrite(&debug2device,(unsigned char)c);
     }
}  

// GSM receive
void USART2_IRQHandler()
{
  if ( USART_GetITStatus(USART2,USART_IT_RXNE) != RESET )
     {
       uint16_t c = USART_ReceiveData(USART2);
       USART_ClearITPendingBit(USART2,USART_IT_RXNE);

       BuffWrite(&device2debug,(unsigned char)c);
     }
}  






void InitUART3()
{
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef usart;
  NVIC_InitTypeDef nvic;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  GPIO_StructInit(&gpio);

  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_Pin = GPIO_Pin_11;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &gpio);

  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_Pin = GPIO_Pin_10;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &gpio);

  GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);

  usart.USART_BaudRate = 9600;
  usart.USART_WordLength = USART_WordLength_8b;
  usart.USART_StopBits = USART_StopBits_1;
  usart.USART_Parity = USART_Parity_No ;
  usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  
  USART_Init(USART3, &usart);

  USART_Cmd(USART3, ENABLE);

  nvic.NVIC_IRQChannel = USART3_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 0;
  nvic.NVIC_IRQChannelSubPriority = 0;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);

  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
}


// GSM
void InitUART2()
{
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef usart;
  NVIC_InitTypeDef nvic;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

  GPIO_StructInit(&gpio);

  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_Pin = GPIO_Pin_6;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOD, &gpio);

  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_Pin = GPIO_Pin_5;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOD, &gpio);

  GPIO_PinAFConfig(GPIOD, GPIO_PinSource6, GPIO_AF_USART2);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_USART2);

  usart.USART_BaudRate = 9600;
  usart.USART_WordLength = USART_WordLength_8b;
  usart.USART_StopBits = USART_StopBits_1;
  usart.USART_Parity = USART_Parity_No ;
  usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  
  USART_Init(USART2, &usart);

  USART_Cmd(USART2, ENABLE);

  nvic.NVIC_IRQChannel = USART2_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 1;
  nvic.NVIC_IRQChannelSubPriority = 1;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);

  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
}


void SendByte(USART_TypeDef* USARTx,volatile TBUFF *i)
{
  if ( i->to_write != i->to_read )
     {
       unsigned char c = i->buff[i->to_read];
       i->to_read++;
       if ( i->to_read == MAX_BUFF )
        i->to_read = 0;

       while( USART_GetFlagStatus(USARTx,USART_FLAG_TXE)==RESET ){}
       USART_SendData(USARTx,c);
     }
}



int main(void)
{
  /* SysTick end of count event each 10ms */
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);

  InitTBuff(&debug2device);
  InitTBuff(&device2debug);

  InitUART3(); // debug
  InitUART2(); // GSM
  
  while (1)
  {
    SendByte(USART2,&debug2device);
    SendByte(USART3,&device2debug);
  }
}


///////////////////



void delay_ms_not_strict(uint32_t ms)
{
  volatile uint32_t nCount;
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  nCount = (RCC_Clocks.HCLK_Frequency / 10000) * ms;
  for(; nCount != 0; nCount --);
}


//------------
static volatile int iTimingDelay = 0;

void msDelay(int ms)
{ 
  iTimingDelay = ms;
  while(iTimingDelay != 0);
}

void TimingDelay_Decrement(void)
{
  int t = iTimingDelay;
  t -= 10;
  t = t < 0 ? 0 : t;
  iTimingDelay = t;
}

//------------


