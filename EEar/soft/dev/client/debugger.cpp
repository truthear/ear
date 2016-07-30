
#include "include.h"



CUART* CDebugger::p_uart = NULL;


void CDebugger::Init(int rate)
{
  assert(!p_uart);
  p_uart = new CBoardUART(BOARD_UART_DEBUG,rate);
}


void CDebugger::PutChar(char c)
{
  assert(p_uart);
  p_uart->SendByte((unsigned char)c);
}


void CDebugger::PutChar(unsigned char c)
{
  assert(p_uart);
  p_uart->SendByte(c);
}


///////////////////////////////


#define MAX_PRINTF_STR_SIZE 1024   // uses stack memory!


extern "C"
int _EXFUN(printf,(const char * __restrict pFormat, ...))
{
  int result = 0;

  char pStr[MAX_PRINTF_STR_SIZE];

  pStr[0] = 0;

  va_list ap;
  va_start(ap, pFormat);
  vsprintf(pStr, pFormat, ap);
  va_end(ap);

  const char *p = pStr;
  while ( 1 )
  {
    char c = *p++;
    if ( c == 0 )
     break;

    if ( c == '\n' )
       {
         CDebugger::PutChar('\r');
       }

    CDebugger::PutChar(c);

    result++;
  }

  return result;
}



