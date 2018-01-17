
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
#include <stdio.h>


//#define SENDER



void delay_ms_not_strict(uint32_t ms)
{
  volatile uint32_t nCount;
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  nCount = (RCC_Clocks.HCLK_Frequency / 10000) * ms;
  for(; nCount != 0; nCount --);
}


#define MAX_BUFF 8192

typedef struct {
unsigned char buff[MAX_BUFF];
unsigned to_write;
unsigned to_read;
} TBUFF;

volatile TBUFF device2debug;

void InitTBuff(volatile TBUFF *i)
{
  i->to_write = 0;
  i->to_read = 0;
}


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


void SendOneByte(USART_TypeDef* USARTx,char c)
{
      while( USART_GetFlagStatus(USARTx,USART_FLAG_TXE)==RESET ){}
      USART_SendData(USARTx,c);
}


char RecvOneByte(USART_TypeDef* USARTx)
{
      while( USART_GetFlagStatus(USARTx,USART_FLAG_RXNE)==RESET ){}
      return (char)USART_ReceiveData(USARTx);
}


// USB debug
void InitUART3()
{
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef usart;

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
}


// RS-485
void InitUART2()
{
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef usart;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

  GPIO_StructInit(&gpio);

  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_Pin = GPIO_Pin_5;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOD, &gpio);

  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_Pin = GPIO_Pin_6;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOD, &gpio);

  gpio.GPIO_Mode = GPIO_Mode_OUT;
  gpio.GPIO_Pin = GPIO_Pin_4;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOD, &gpio);
  GPIOD->BSRRL = GPIO_Pin_4;  // set high

  GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_USART2);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource6, GPIO_AF_USART2);

  usart.USART_BaudRate = 9600;
  usart.USART_WordLength = USART_WordLength_8b;
  usart.USART_StopBits = USART_StopBits_1;
  usart.USART_Parity = USART_Parity_No ;
  usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  
  USART_Init(USART2, &usart);

  USART_Cmd(USART2, ENABLE);
}


// lora receive
void USART6_IRQHandler()
{
  if ( USART_GetITStatus(USART6,USART_IT_RXNE) != RESET )
     {
       USART_ClearITPendingBit(USART6,USART_IT_RXNE);
       uint16_t c = USART_ReceiveData(USART6);
       
       BuffWrite(&device2debug,(unsigned char)c);
     }
}  


// microchip
void InitUART6()
{
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef usart;
  NVIC_InitTypeDef nvic;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

  GPIO_StructInit(&gpio);

  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_Pin = GPIO_Pin_6;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOC, &gpio);

  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_Pin = GPIO_Pin_7;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOC, &gpio);

  GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);

  usart.USART_BaudRate = 57600;
  usart.USART_WordLength = USART_WordLength_8b;
  usart.USART_StopBits = USART_StopBits_1;
  usart.USART_Parity = USART_Parity_No ;
  usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  
  USART_Init(USART6, &usart);

  USART_Cmd(USART6, ENABLE);

  nvic.NVIC_IRQChannel = USART6_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 1;
  nvic.NVIC_IRQChannelSubPriority = 1;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);

  USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);

  // reset pin
  gpio.GPIO_Mode = GPIO_Mode_OUT;
  gpio.GPIO_Pin = GPIO_Pin_2;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOE, &gpio);
  GPIOE->BSRRL = GPIO_Pin_2;  // set high
}



void SendString(USART_TypeDef* USARTx,const char *s)
{
  while ( *s )
  {
    char c = *s++;
    
    SendOneByte(USARTx,c);

    //delay_ms_not_strict(10);
  }
}


int GetByteFromBuff(volatile TBUFF *i)
{
  int rc = 0;
  
  if ( i->to_write != i->to_read )
     {
       unsigned char c = i->buff[i->to_read];
       i->to_read++;
       if ( i->to_read == MAX_BUFF )
        i->to_read = 0;

       rc = c;
     }

  return rc;
}


int IsBufferEmpty(volatile TBUFF *i)
{
  return i->to_write == i->to_read;
}


void Cmd(const char *cmd)
{
  if ( cmd )
     {
       SendString(USART6,cmd);
       SendString(USART6,"\r\n");

       SendString(USART3,cmd);
       SendString(USART3,"\r\n");

       SendString(USART2,cmd);
       SendString(USART2,"\r\n");
     }

  while ( 1 )
  {
    char c = GetByteFromBuff(&device2debug);
    if ( c )
       {
         SendOneByte(USART3,c);
         SendOneByte(USART2,c);
    
         if ( c == '\n' /*&& IsBufferEmpty(&device2debug)*/ )
          break;
       }
  }

  STM_EVAL_LEDOn(LED5);
  delay_ms_not_strict(100);
  STM_EVAL_LEDOff(LED5);
}





int main(void)
{
  InitTBuff(&device2debug);

  delay_ms_not_strict(1000);  // time to USB port connected

  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  STM_EVAL_LEDInit(LED5);
  STM_EVAL_LEDInit(LED6);

  InitUART3(); // USB-debug
  InitUART2(); // RS-485
  InitUART6(); // lora

  Cmd("sys get ver");
  Cmd("sys get vdd");
  Cmd("sys get hweui");
  Cmd("mac pause");
  Cmd("radio set mod lora");
  Cmd("radio set freq 868000000");
  Cmd("radio set sf sf7");
  Cmd("radio set prlen 8");
  Cmd("radio set crc on");
  Cmd("radio set iqi off");
  Cmd("radio set cr 4/5");
  Cmd("radio set bw 500");
  Cmd("radio set sync 12");
  Cmd("radio set wdt 5000");
  Cmd("radio set pwr 15");
  Cmd("radio get pwr");

  while (1)
  {
    Cmd("mac pause");

    #ifdef SENDER
    Cmd("radio tx 48484848484848484848484848484848484848484848484848484848484848484848484848484848");
    Cmd(NULL);
    delay_ms_not_strict(800);
    #else
    Cmd("radio rx 0");
    Cmd(NULL);
    #endif

  }
  
}



