
#include "include.h"


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


void SendByte(CUART *uart,volatile TBUFF *i)
{
  if ( i->to_write != i->to_read )
     {
       unsigned char c = i->buff[i->to_read];
       i->to_read++;
       if ( i->to_read == MAX_BUFF )
        i->to_read = 0;

       uart->SendByte(c);
     }
}

// handler
void OnUARTRecvByte(void* p,unsigned char data)
{
  BuffWrite((volatile TBUFF*)p,data);
}


void SendStringWithCRC(CUART *uart,const char *cmd)
{
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

  for ( unsigned n = 0; n < sizeof(s); n++ )
      {
        char c = s[n];
        if ( c == 0 )
         break;

        uart->SendByte(c);
      }
}


void OnButton(void *p)
{
//  reinterpret_cast<CLED*>(led)->On();
//  CCPUTicks::Delay(100);
//  reinterpret_cast<CLED*>(led)->Off();

  SendStringWithCRC(reinterpret_cast<CUART*>(p),"$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
}


int main()
{
  InitTBuff(&debug2device);
  InitTBuff(&device2debug);
  
  CSysTicks::Init();
  CCPUTicks::Init();

  CBoardLED led(BOARD_LED4);

  const int rate = 9600;
  
  CUART *dev = new CBoardUART(BOARD_UART_GSM,rate,true,true,OnUARTRecvByte,(void*)&device2debug);
  CUART *dbg = new CBoardUART(BOARD_UART_DEBUG,rate,true,true,OnUARTRecvByte,(void*)&debug2device);
  //CDebugger::Init();

  //new CBoardButton(BOARD_BUTTON1,OnButton,dev);
  //new CBoardButton(BOARD_BUTTON2,OnButton,new CBoardLED(BOARD_LED2));
  //new CBoardButton(BOARD_BUTTON3,OnButton,new CBoardLED(BOARD_LED3));

  while (1)
  {
    SendByte(dev,&debug2device);
    SendByte(dbg,&device2debug);
  }
}
