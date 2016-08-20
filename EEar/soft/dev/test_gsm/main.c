
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
#include <stdio.h>


//         RX   TX
// ------------------------
// USART2:  PD6  PD5  - GSM
// USART3:  PB11 PB10 - debug !
// ------------------------


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

volatile TBUFF debug2device;
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
       USART_ClearITPendingBit(USART2,USART_IT_RXNE);
       uint16_t c = USART_ReceiveData(USART2);
       
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

void GSM_Reset_Init()
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void GSM_Reset_ON()
{
  GPIOB->BSRRL = GPIO_Pin_0;
}

void GSM_Reset_Off()
{
  GPIOB->BSRRH = GPIO_Pin_0;  
}


void SendString(USART_TypeDef* USARTx,const char *s,int delay_ms)
{
  while ( *s )
  {
    char c = *s++;
    
    SendOneByte(USARTx,c);
    
    if ( delay_ms > 0 )
    {
      delay_ms_not_strict(delay_ms);
    }
  }
}


#define BUTTON1_PIN                GPIO_Pin_9
#define BUTTON1_GPIO_PORT          GPIOD
#define BUTTON1_GPIO_CLK           RCC_AHB1Periph_GPIOD
#define BUTTON1_EXTI_LINE          EXTI_Line9
#define BUTTON1_EXTI_PORT_SOURCE   EXTI_PortSourceGPIOD
#define BUTTON1_EXTI_PIN_SOURCE    EXTI_PinSource9
#define BUTTON1_EXTI_IRQn          EXTI9_5_IRQn

#define BUTTON2_PIN                GPIO_Pin_10
#define BUTTON2_GPIO_PORT          GPIOD
#define BUTTON2_GPIO_CLK           RCC_AHB1Periph_GPIOD
#define BUTTON2_EXTI_LINE          EXTI_Line10
#define BUTTON2_EXTI_PORT_SOURCE   EXTI_PortSourceGPIOD
#define BUTTON2_EXTI_PIN_SOURCE    EXTI_PinSource10
#define BUTTON2_EXTI_IRQn          EXTI15_10_IRQn

#define BUTTON3_PIN                GPIO_Pin_11
#define BUTTON3_GPIO_PORT          GPIOD
#define BUTTON3_GPIO_CLK           RCC_AHB1Periph_GPIOD
#define BUTTON3_EXTI_LINE          EXTI_Line11
#define BUTTON3_EXTI_PORT_SOURCE   EXTI_PortSourceGPIOD
#define BUTTON3_EXTI_PIN_SOURCE    EXTI_PinSource11
#define BUTTON3_EXTI_IRQn          EXTI15_10_IRQn


static GPIO_TypeDef* BUTTON_PORT[]        = {BUTTON1_GPIO_PORT        ,BUTTON2_GPIO_PORT        ,BUTTON3_GPIO_PORT        }; 
static const uint16_t BUTTON_PIN[]        = {BUTTON1_PIN              ,BUTTON2_PIN              ,BUTTON3_PIN              }; 
static const uint32_t BUTTON_CLK[]        = {BUTTON1_GPIO_CLK         ,BUTTON2_GPIO_CLK         ,BUTTON3_GPIO_CLK         };
static const uint16_t BUTTON_EXTI_LINE[]  = {BUTTON1_EXTI_LINE        ,BUTTON2_EXTI_LINE        ,BUTTON3_EXTI_LINE        };
static const uint8_t BUTTON_PORT_SOURCE[] = {BUTTON1_EXTI_PORT_SOURCE ,BUTTON2_EXTI_PORT_SOURCE ,BUTTON3_EXTI_PORT_SOURCE };
static const uint8_t BUTTON_PIN_SOURCE[]  = {BUTTON1_EXTI_PIN_SOURCE  ,BUTTON2_EXTI_PIN_SOURCE  ,BUTTON3_EXTI_PIN_SOURCE  }; 
static const uint8_t BUTTON_IRQn[]        = {BUTTON1_EXTI_IRQn        ,BUTTON2_EXTI_IRQn        ,BUTTON3_EXTI_IRQn        };


void ButtonInit(int Button)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable the BUTTON Clock */
  RCC_AHB1PeriphClockCmd(BUTTON_CLK[Button], ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  /* Configure Button pin as input */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = BUTTON_PIN[Button];
  GPIO_Init(BUTTON_PORT[Button], &GPIO_InitStructure);

    /* Connect Button EXTI Line to Button GPIO Pin */
    SYSCFG_EXTILineConfig(BUTTON_PORT_SOURCE[Button], BUTTON_PIN_SOURCE[Button]);

    /* Configure Button EXTI line */
    EXTI_InitStructure.EXTI_Line = BUTTON_EXTI_LINE[Button];
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = BUTTON_IRQn[Button];
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure); 
}


void EXTI9_5_IRQHandler()
{
  if ( EXTI_GetITStatus(BUTTON1_EXTI_LINE) != RESET )
  {
    EXTI_ClearITPendingBit(BUTTON1_EXTI_LINE);  
    
  STM_EVAL_LEDOff(LED5);

  GSM_Reset_ON();
  delay_ms_not_strict(300);
  GSM_Reset_Off(); // Off
//  delay_ms_not_strict(1000);

  STM_EVAL_LEDOn(LED5);

    
  }
}


int main(void)
{
  InitTBuff(&debug2device);
  InitTBuff(&device2debug);

  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  STM_EVAL_LEDInit(LED5);
  STM_EVAL_LEDInit(LED6);

  ButtonInit(0);
//  ButtonInit(1);
//  ButtonInit(2);

  InitUART3(); // debug
  InitUART2(); // GSM

  GSM_Reset_Init();
  GSM_Reset_Off(); // Off by default, but needed (not worked when ON)


//  delay_ms_not_strict(300);  // here symbol 0 can be sent after power_on, at least 200 msec need to wait
                             // after hardware reset no 0 will sent

  
  //delay_ms_not_strict(3000);
  





//  delay_ms_not_strict(1000);
//  GSM_Reset_ON();
//  delay_ms_not_strict(300);
//  GSM_Reset_Off(); // Off
//  delay_ms_not_strict(1000);

  STM_EVAL_LEDOn(LED5);
  

  while (1)
  {
    SendByte(USART2,&debug2device);
    SendByte(USART3,&device2debug);
  }
  
  
//  SendString(USART2,"ATE\r",0);
//  SendString(USART2,"AT+IPR=9600\r",0);
//  delay_ms_not_strict(100);
  
//  SendString(USART2,"AT+CMEE=2\r",0);
//  delay_ms_not_strict(500);
//  SendString(USART2,"AT+CPIN?\r",0);
//  delay_ms_not_strict(500);
//  SendString(USART2,"AT+COPS=1,2,\"25506\"\r",0);
//  SendString(USART2,"AT+COPS=0\r",0);
//  delay_ms_not_strict(500);

  
//25502	Beeline	
//25523	CDMA 	
//25505	Golden Telecom	
//25504	IT	
//25503	Kyivstar	
//25506	life:)	
//25501	MTS	
//25521	PEOPLEnet	
//25507	Utel	  
  
/*  
  delay_ms_not_strict(20000);

  SendString(USART2,"AT+CREG?\r",0);
  delay_ms_not_strict(500);
  SendString(USART2,"AT+CGREG?\r",0);
  delay_ms_not_strict(500);
  SendString(USART2,"AT+COPS?\r",0);
  delay_ms_not_strict(500);
  SendString(USART2,"AT+CSQ\r",0);
  delay_ms_not_strict(500);

  SendString(USART2,"AT+CGDCONT=1,\"IP\",\"internet\"\r",0);
  delay_ms_not_strict(500);
  
  SendString(USART2,"AT#SGACT=1,1\r",0);
  delay_ms_not_strict(7000);

  SendString(USART2,"AT#HTTPCFG=1,\"195.234.5.137\",80\r",0);
  delay_ms_not_strict(500);
  SendString(USART2,"AT#HTTPQRY=1,0,\"/\"\r",0);
  delay_ms_not_strict(7000);

  SendString(USART2,"AT+CGPADDR=1\r",0);
  delay_ms_not_strict(10000);
  SendString(USART2,"AT+CGATT=0\r",0);
  delay_ms_not_strict(10000);
  SendString(USART2,"AT+CGPADDR=1\r",0);
  delay_ms_not_strict(10000);
  SendString(USART2,"AT#SGACT=1,1\r",0);
  delay_ms_not_strict(10000);
  SendString(USART2,"AT#SGACT=1,1\r",0);
  delay_ms_not_strict(10000);
  SendString(USART2,"AT+CGPADDR=1\r",0);
  delay_ms_not_strict(10000);

  SendString(USART2,"AT#HTTPQRY=1,0,\"/aaa\"\r",0);
  delay_ms_not_strict(7000);
  */
  
//  SendString(USART2,"AT#SD=1,0,80,\"195.234.5.137\",0,0,1\r",0);
//  delay_ms_not_strict(5000);
//  SendString(USART2,"AT#SSEND=1\r",0);
//  delay_ms_not_strict(500);
//  SendString(USART2,"GET / HTTP/1.1\r\nHost: 195.234.5.137\r\n\r\n\x1A",0);
//  delay_ms_not_strict(5000);

//  SendString(USART2,"GET / HTTP/1.1\r\nHost: 195.234.5.137\r\n\r\n",0);
  //  delay_ms_not_strict(5000);
  
//  SendString(USART2,"AT+CSQ\r",0);
//  delay_ms_not_strict(500);
//  SendString(USART2,"ATD+380918029286;\r",0);
//  delay_ms_not_strict(5000);
//  SendString(USART2,"AT+CMGS=+380918029286,145\r",0);
//  delay_ms_not_strict(500);
//  SendString(USART2,"Test\x1A",0);
//  delay_ms_not_strict(15000);
    
  
  
}



