
#include "include.h"


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


void SendOneByte(USART_TypeDef* USARTx,char c)
{
      while( USART_GetFlagStatus(USARTx,USART_FLAG_TXE)==RESET ){}
      USART_SendData(USARTx,c);
}


void DbgChar(char c)
{
  SendOneByte(USART3,c);
}


extern "C"
int _EXFUN(printf, (const char* __restrict pFormat, ...))
{
    int result = 0;

    va_list ap;
    va_start(ap, pFormat);

    char pStr[1024];
    vsprintf(pStr, pFormat, ap);

    const char *p = pStr;
    while ( 1 )
    {
      char c = *p++;
      if ( c == 0 )
       break;

      result++;

      if ( c == '\n' )
         {
           DbgChar('\r');
         }

      DbgChar(c);
    }

    va_end(ap);

    return result;
}
