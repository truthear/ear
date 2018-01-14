
#include "include.h"



bool CUART::IsDataAvailForRecv()
{
  return (USART_GetFlagStatus(USARTx,USART_FLAG_RXNE) != RESET);
}


bool CUART::IsBuffEmptyForSend()
{
  return (USART_GetFlagStatus(USARTx,USART_FLAG_TXE) != RESET);
}


unsigned char CUART::RecvByte()
{
  while ( !IsDataAvailForRecv() ) {}
  return (unsigned char)USART_ReceiveData(USARTx);
}


void CUART::SendByte(unsigned char data)
{
  while ( !IsBuffEmptyForSend() ) {}
  USART_SendData(USARTx,data);
}




typedef void (*TPeriphClkCmdFunc)(uint32_t,FunctionalState);

static USART_TypeDef*    UART_NUM[]         = { USART1                  , USART2                 , USART3                 };
static TPeriphClkCmdFunc UART_PERIPH_FUNC[] = { RCC_APB2PeriphClockCmd  , RCC_APB1PeriphClockCmd , RCC_APB1PeriphClockCmd };
static uint32_t          UART_PERIPH[]      = { RCC_APB2Periph_USART1   , RCC_APB1Periph_USART2  , RCC_APB1Periph_USART3  };
static CPin::EPins       UART_PIN1[]        = { CPin::PB_7              , CPin::PD_6             , CPin::PB_11            };
static CPin::EPins       UART_PIN2[]        = { CPin::PB_6              , CPin::PD_5             , CPin::PB_10            };
static uint8_t           UART_AF[]          = { GPIO_AF_USART1          , GPIO_AF_USART2         , GPIO_AF_USART3         };
static IRQn              UART_IRQn[]        = { USART1_IRQn             , USART2_IRQn            , USART3_IRQn            };


CBoardUART::TUARTRXCALLBACKWITHPARM CBoardUART::callbacks[3] = {{NULL,NULL},{NULL,NULL},{NULL,NULL}};



CBoardUART::CBoardUART(EBoardUarts uart,int rate,bool allow_rx,bool allow_tx,
                       TUARTRXCALLBACK cbrx,void *cbparm,int irq_priority)
   : CUART(UART_NUM[uart])
{
  USART_TypeDef *USARTx = UART_NUM[uart];
  
  UART_PERIPH_FUNC[uart](UART_PERIPH[uart],ENABLE);

  CPin::InitAsAF(UART_PIN1[uart],UART_AF[uart],GPIO_PuPd_UP);
  CPin::InitAsAF(UART_PIN2[uart],UART_AF[uart],GPIO_PuPd_UP);
  
  USART_InitTypeDef ui;
  ui.USART_BaudRate = rate;
  ui.USART_WordLength = USART_WordLength_8b;
  ui.USART_StopBits = USART_StopBits_1;
  ui.USART_Parity = USART_Parity_No;
  ui.USART_Mode = (allow_rx?USART_Mode_Rx:0) | (allow_tx?USART_Mode_Tx:0);
  ui.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  
  USART_Init(USARTx, &ui);

  USART_Cmd(USARTx,ENABLE);

  if ( allow_rx && cbrx )
     {
       callbacks[uart].func = cbrx;
       callbacks[uart].parm = cbparm;
       
       NVIC_InitTypeDef nvic;
       nvic.NVIC_IRQChannel = UART_IRQn[uart];
       nvic.NVIC_IRQChannelPreemptionPriority = irq_priority;
       nvic.NVIC_IRQChannelSubPriority = 0;
       nvic.NVIC_IRQChannelCmd = ENABLE;
       NVIC_Init(&nvic);

       USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
     }
}


CBoardUART::~CBoardUART()
{
}


void CBoardUART::OnIRQ_Internal(EBoardUarts uart)
{
  USART_TypeDef *USARTx = UART_NUM[uart];
  
  if ( USART_GetITStatus(USARTx,USART_IT_RXNE) != RESET )
     {
       uint16_t c = USART_ReceiveData(USARTx);  // this also clears pending IT bit!

       const TUARTRXCALLBACKWITHPARM& i = callbacks[uart];
       if ( i.func )
          {
            i.func(i.parm,(unsigned char)c);
          }
     }
}


extern "C"
void USART1_IRQHandler()
{
  CBoardUART::OnIRQ_Internal(BOARD_UART1);
}  


extern "C"
void USART2_IRQHandler()
{
  CBoardUART::OnIRQ_Internal(BOARD_UART2);
}  


extern "C"
void USART3_IRQHandler()
{
  CBoardUART::OnIRQ_Internal(BOARD_UART3);
}  




