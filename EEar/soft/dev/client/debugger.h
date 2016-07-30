
#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__


class CUART;


class CDebugger
{
          static CUART *p_uart;

  public:
          static void Init(int rate=9600);
          static void PutChar(char c);
          static void PutChar(unsigned char c);
};



#endif
