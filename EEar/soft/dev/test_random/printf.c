
#include <stdio.h>
#include <stdarg.h>
#include "dbg_uart.h"



int printf(const char *pFormat, ...)
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

