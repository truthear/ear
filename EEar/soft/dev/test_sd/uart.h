
#ifndef __UART_H__
#define __UART_H__


class CUART
{
          USART_TypeDef *USARTx;

  protected:
          CUART(USART_TypeDef *x) : USARTx(x) {}
  public:
          virtual ~CUART() {}

          bool IsDataAvailForRecv();         // async function
          bool IsBuffEmptyForSend();         // async function
          unsigned char RecvByte();          // sync function, waits for IsDataAvailForRecv()
          void SendByte(unsigned char data); // sync function, waits for IsBuffEmptyForSend()
};



enum EBoardUarts {
BOARD_UART1      = 0,
BOARD_UART2      = 1,
BOARD_UART3      = 2,
BOARD_UART_GPS   = BOARD_UART1,
BOARD_UART_GSM   = BOARD_UART2,
BOARD_UART_DEBUG = BOARD_UART3,
};


class CBoardUART : public CUART
{
  public:
          typedef void (*TUARTRXCALLBACK)(void*,unsigned char data);

  private:        
          typedef struct {
          TUARTRXCALLBACK func;
          void *parm;
          } TUARTRXCALLBACKWITHPARM;

          static TUARTRXCALLBACKWITHPARM callbacks[3];

  public:
          CBoardUART(EBoardUarts uart,int rate,bool allow_rx=true,bool allow_tx=true,
                     TUARTRXCALLBACK cbrx=NULL,void *cbparm=NULL,int irq_priority=7);
          ~CBoardUART();

  public:
          // used internally
          static void OnIRQ_Internal(EBoardUarts uart);
};


#endif

