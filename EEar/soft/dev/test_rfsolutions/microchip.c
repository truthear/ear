#include "board.h"


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


static void SendOneByte(USART_TypeDef* USARTx,char c)
{
      while( USART_GetFlagStatus(USARTx,USART_FLAG_TXE)==RESET ){}
      USART_SendData(USARTx,c);
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
}



static void SendString(USART_TypeDef* USARTx,const char *s)
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


void EmptyBuffer(volatile TBUFF *i)
{
  i->to_write = i->to_read = 0;
}


static void Cmd(const char *cmd)
{
  EmptyBuffer(&device2debug);

  SendString(USART6,cmd);
  SendString(USART6,"\r\n");

  while ( 1 )
  {
    char c = GetByteFromBuff(&device2debug);
    if ( c )
       {
         //printf("%c",c);
         
         if ( c == '\n' && IsBufferEmpty(&device2debug) )
          break;
       }
  }
}


void Microchip_Init()
{
  InitTBuff(&device2debug);

  InitUART6(); // lora

  GPIO_InitTypeDef gpio;
  gpio.GPIO_Mode = GPIO_Mode_OUT;
  gpio.GPIO_Pin = GPIO_Pin_2;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOE, &gpio);
  GPIO_SetBits(GPIOE,GPIO_Pin_2);  // set 1

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
  Cmd("radio set pwr 15");
}


void Microchip_Send(const char *src)
{
  Cmd("mac pause");
  
  char cmd[600];

  strcpy(cmd,"radio tx ");

  while ( *src )
  {
    int c = (int)(unsigned char)(*src++);

    char hxx[5];

    sprintf(hxx,"%02X",c);

    strcat(cmd,hxx);
  }

  Cmd(cmd);
}

