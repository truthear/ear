#include <stm32f4xx_conf.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_usart.h>
#include <stdio.h>


GPIO_InitTypeDef  GPIO_InitStructure;

uint8_t RxData[256];
uint8_t TxData[256];
volatile uint8_t RxIn = 0;
volatile uint8_t RxOut = 0;
volatile uint8_t TxIn = 0;
volatile uint8_t TxOut = 0;
//uint8_t RxEmpty=1;
uint32_t cnt=0;
void UsartTx(){
    if(TxIn != TxOut){
        uint8_t tmp=TxData[TxOut++];
        USART_SendData(USART3, tmp);
    }
}
void UsartSend(){
    //if((USART3->SR & USART_SR_TXE) && (TxIn != TxOut)){
    if((USART_GetFlagStatus(USART3,USART_FLAG_TXE)!= RESET) && (TxIn != TxOut)){
        UsartTx();
    }
}
void UsartRx(){
    int tmp=RxIn-RxOut;
    if(tmp<256 && tmp!=-1){
        RxData[RxIn++]=USART_ReceiveData(USART3);
    }
}
uint8_t UsartTestData(){
    return RxIn!=RxOut;
}
uint8_t UsartGetData(){
    return RxData[RxOut++];
}
char UsartPutData(char Dt){
    int tmp=TxIn-TxOut;
    if(tmp<255 && tmp!=-1){
        TxData[TxIn++]=Dt;
        return 1;
    }
    return 0;
}



void Delay(__IO uint32_t nCount);


void InitUART3()
{
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef usart;
  NVIC_InitTypeDef nvic;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  GPIO_StructInit(&gpio);

  gpio.GPIO_Mode    = GPIO_Mode_AF;
  gpio.GPIO_Pin     = GPIO_Pin_11;
  gpio.GPIO_Speed   = GPIO_Speed_50MHz;
  gpio.GPIO_OType   = GPIO_OType_PP;
  gpio.GPIO_PuPd    = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &gpio);

  gpio.GPIO_Mode    = GPIO_Mode_AF;
  gpio.GPIO_Pin     = GPIO_Pin_10;
  gpio.GPIO_Speed   = GPIO_Speed_50MHz;
  gpio.GPIO_OType   = GPIO_OType_PP;
  gpio.GPIO_PuPd    = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &gpio);

  GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);

  usart.USART_BaudRate              = 9600;
  usart.USART_WordLength            = USART_WordLength_8b;
  usart.USART_StopBits              = USART_StopBits_1;
  usart.USART_Parity                = USART_Parity_No ;
  usart.USART_Mode                  = USART_Mode_Rx | USART_Mode_Tx;
  usart.USART_HardwareFlowControl   = USART_HardwareFlowControl_None;
  USART_Init(USART3, &usart);

  USART_Cmd(USART3, ENABLE);

  nvic.NVIC_IRQChannel                      = USART3_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority    = 0;
  nvic.NVIC_IRQChannelSubPriority           = 0;
  nvic.NVIC_IRQChannelCmd                   = ENABLE;
  NVIC_Init(&nvic);

  USART_ITConfig(USART3, USART_IT_TC, ENABLE);
  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
}

void initAll()
{
    __enable_irq();
    InitUART3();
}
#define LED_4 (1<<12)
#define LED_3 (1<<13)
#define LED_2 (1<<14)
#define LED_1 (1<<15)

int main(void)
{

  SystemInit();
  initAll();

  /* GPIOD Periph clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

  /* Configure PD12, PD13, PD14 and PD15 in output pushpull mode */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  printf("Hello world form EEar %d\n",1234);

  while(1)
      {
        GPIOD->ODR ^=LED_1;
          if(UsartTestData()){
              uint8_t tmp=UsartGetData();
              printf("Test:\"%c\"\n",tmp);
              UsartSend();
          }

      }


}



void USART3_IRQHandler()
{
  if (USART_GetITStatus(USART3, USART_IT_TC) != RESET)
  {
      USART_ClearITPendingBit(USART3, USART_IT_TC);
      UsartTx();
  }

  if(USART_GetITStatus(USART3, USART_IT_RXNE)!=RESET)
  {
      UsartRx();
    }
}

void Delay(__IO uint32_t nCount)
{
  while(nCount--)
  {
  }
}

