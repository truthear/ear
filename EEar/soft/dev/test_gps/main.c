
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
#include <stdio.h>
#include <string.h>


//#define RTC_USE_LSI
#define RTC_USE_LSE

#define RTC_ASYNC_PREDIV  0x3
#define RTC_SYNC_PREDIV   0x1FFF
// должно выполняться: (RTC_ASYNC_PREDIV+1)*(RTC_SYNC_PREDIV+1)=32768
// RTC_SYNC_PREDIV - это фактически счетчик SubSecond


#define TIMESTAMP_EXTI_LINE   EXTI_Line21


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


// GPS receive
void USART1_IRQHandler()
{
  if ( USART_GetITStatus(USART1,USART_IT_RXNE) != RESET )
     {
       uint16_t c = USART_ReceiveData(USART1);
       USART_ClearITPendingBit(USART1,USART_IT_RXNE);

       BuffWrite(&device2debug,(unsigned char)c);
     }
}  



void TAMP_STAMP_IRQHandler()
{
  if ( EXTI_GetITStatus(TIMESTAMP_EXTI_LINE) != RESET ) 
     {
       // пока не очистим эти флаги вызываться прерывание не будет снова!
       // флаг TS overflow установлен только если при неочищенном TS event выдается прерывание
       RTC_ClearFlag(RTC_FLAG_TSOVF);  // TS overflow
       RTC_ClearFlag(RTC_FLAG_TSF);  // TS event, - this cause clearing TS date,time,subs registers!
       
       EXTI_ClearITPendingBit(TIMESTAMP_EXTI_LINE);

       STM_EVAL_LEDOn(LED5);
       delay_ms_not_strict(50);
       STM_EVAL_LEDOff(LED5);
       delay_ms_not_strict(50);
     }
}



void RTC_Config()
{
  /* Enable the PWR clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

  /* Allow access to RTC */
  PWR_BackupAccessCmd(ENABLE);

  RCC_BackupResetCmd(ENABLE);
  RCC_BackupResetCmd(DISABLE);
  
#ifdef RTC_USE_LSI
  /* LSI used as RTC source clock */
  /* The RTC Clock may varies due to LSI frequency dispersion. */   
  /* Enable the LSI OSC */ 
  RCC_LSICmd(ENABLE);

  /* Wait till LSI is ready */  
  while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {
  }

  /* Select the RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
#else
  /* Enable the LSE OSC */
  RCC_LSEConfig(RCC_LSE_ON);

  /* Wait till LSE is ready */  
  while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
  {
  }

  /* Select the RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
#endif
  
  /* Enable the RTC Clock */
  RCC_RTCCLKCmd(ENABLE);

  /* Wait for RTC APB registers synchronisation */
  RTC_WaitForSynchro();

  /* Calendar Configuration with LSI supposed at 32KHz */
  {
    RTC_InitTypeDef RTC_InitStruct;
    RTC_InitStruct.RTC_AsynchPrediv = RTC_ASYNC_PREDIV;
    RTC_InitStruct.RTC_SynchPrediv  = RTC_SYNC_PREDIV;
    RTC_InitStruct.RTC_HourFormat = RTC_HourFormat_24;
    RTC_Init(&RTC_InitStruct);  
  }

  {
    RTC_DateTypeDef RTC_DateStruct;
    RTC_DateStruct.RTC_WeekDay = 7;
    RTC_DateStruct.RTC_Date = 0x06;
    RTC_DateStruct.RTC_Month = 0x12;
    RTC_DateStruct.RTC_Year = 0x15;
    RTC_SetDate(RTC_Format_BCD, &RTC_DateStruct);
  }

  {
    RTC_TimeTypeDef RTC_TimeStruct;
    RTC_TimeStruct.RTC_Hours = 0x09;
    RTC_TimeStruct.RTC_Minutes = 0x30;
    RTC_TimeStruct.RTC_Seconds = 0x00;
    RTC_TimeStruct.RTC_H12 = 0;
    RTC_SetTime(RTC_Format_BCD, &RTC_TimeStruct);
  }


  {
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = TIMESTAMP_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
  }

  {
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TAMP_STAMP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 15;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure); 
  }

  RTC_TimeStampPinSelection(RTC_TimeStampPin_PC13);  // PC13 is default

  RTC_ClearFlag(RTC_FLAG_TSOVF);  // TS overflow
  RTC_ClearFlag(RTC_FLAG_TSF);  // TS event

  RTC_ITConfig(RTC_IT_TS, ENABLE);  // enable TS interrupt

  RTC_TimeStampCmd(RTC_TimeStampEdge_Rising, ENABLE);    // enable TS on rising edge
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

  usart.USART_BaudRate = 115200;
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


// GPS
void InitUART1()
{
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef usart;
  NVIC_InitTypeDef nvic;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  GPIO_StructInit(&gpio);

  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_Pin = GPIO_Pin_7;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &gpio);

  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_Pin = GPIO_Pin_6;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &gpio);

  GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1);

  usart.USART_BaudRate = 115200;
  usart.USART_WordLength = USART_WordLength_8b;
  usart.USART_StopBits = USART_StopBits_1;
  usart.USART_Parity = USART_Parity_No ;
  usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  
  USART_Init(USART1, &usart);

  USART_Cmd(USART1, ENABLE);

  nvic.NVIC_IRQChannel = USART1_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 1;
  nvic.NVIC_IRQChannelSubPriority = 1;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);

  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
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


void SendStringWithCRC(const char *cmd)
{
  int n;
  char s[200];
  char s_crc[16];

  unsigned char crc = 0;

  strcpy(s,cmd);

  while (1)
  {
    char c = *cmd++;
    if ( c == 0 )
      break;
    
    if ( c != '$' )
     crc ^= (unsigned char)c;
  }

  sprintf(s_crc,"*%02X\r\n",crc);
  strcat(s,s_crc);

  for ( n = 0; n < sizeof(s); n++ )
      {
        char c = s[n];
        if ( c == 0 )
         break;

         while( USART_GetFlagStatus(USART1,USART_FLAG_TXE)==RESET ){}
         USART_SendData(USART1,c);
      }
}


void EXTI9_5_IRQHandler()
{
  if ( EXTI_GetITStatus(BUTTON1_EXTI_LINE) != RESET )
  {
    STM_EVAL_LEDOn(LED3);
    delay_ms_not_strict(100);
    STM_EVAL_LEDOff(LED3);

    SendStringWithCRC("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
      
    EXTI_ClearITPendingBit(BUTTON1_EXTI_LINE);  
  }
}


void EXTI15_10_IRQHandler()
{
  static int reset_mode = 1;
  
  if ( EXTI_GetITStatus(BUTTON2_EXTI_LINE) != RESET )
  {
    char s[100];
    
    STM_EVAL_LEDOn(LED4);
    delay_ms_not_strict(100);
    STM_EVAL_LEDOff(LED4);

    sprintf(s,"$PMTK10%d",reset_mode);
    reset_mode++;
    if ( reset_mode == 5 )
     reset_mode = 1;
    
    SendStringWithCRC(s);
    
    EXTI_ClearITPendingBit(BUTTON2_EXTI_LINE);  
  }
  
  if ( EXTI_GetITStatus(BUTTON3_EXTI_LINE) != RESET )
  {
//    SendStringWithCRC("$PMTK314,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1");
    SendStringWithCRC("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    SendStringWithCRC("$PMTK353,1,0,0,0,0");  // GPS only

  
    EXTI_ClearITPendingBit(BUTTON3_EXTI_LINE);  
  }
  
}


int main(void)
{
  /* SysTick end of count event each 10ms */
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);

  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  STM_EVAL_LEDInit(LED5);
  STM_EVAL_LEDInit(LED6);

  ButtonInit(0);
  ButtonInit(1);
  ButtonInit(2);

  RTC_Config();

  InitTBuff(&debug2device);
  InitTBuff(&device2debug);

  InitUART3(); // debug
  InitUART1(); // GPS


  //msDelay(1800);
  //SendStringWithCRC("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
  //SendStringWithCRC("$PMTK353,1,0,0,0,0");  // GPS only

  STM_EVAL_LEDOn(LED4);

  
  while (1)
  {
    SendByte(USART1,&debug2device);
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


void SysTick_Handler(void)
{
  TimingDelay_Decrement();
}

//------------


