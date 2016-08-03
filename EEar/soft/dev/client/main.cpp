
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


void TestSectorRW(unsigned start_sector)
{
  const unsigned num_sectors = 4;
  
  char data[CSDCard::SECTOR_SIZE*num_sectors];
  char data2[CSDCard::SECTOR_SIZE*num_sectors];
  for ( unsigned n = 0; n < sizeof(data); n++ )
  {
    data[n] = (char)n;
  }

  if ( !CSDCard::Write(data,start_sector,num_sectors) )
     {
       printf("Write failed\n");
     }
  else
     {
       if ( !CSDCard::Read(data2,start_sector,num_sectors) )
          {
            printf("Read failed\n");
          }
       else
          {
            if ( memcmp(data,data2,sizeof(data)) == 0 )
               {
                 printf("OK!\n");
               }
            else
               {
                 printf("cmp failed\n");
               }
          }
     }
}


void TestSectorR(unsigned start_sector)
{
  const unsigned num_sectors = 4;
  
  char data[CSDCard::SECTOR_SIZE*num_sectors];

     {
       if ( !CSDCard::Read(data,start_sector,num_sectors) )
          {
            printf("Read failed\n");
          }
       else
          {
            printf("OK!\n");
          }
     }
}



int main()
{
//  InitTBuff(&debug2device);
//  InitTBuff(&device2debug);
  
  CSysTicks::Init();
  CCPUTicks::Init();


//  const int rate = 9600;
  
//  CUART *dev = new CBoardUART(BOARD_UART_GSM,rate,true,true,OnUARTRecvByte,(void*)&device2debug);
//  CUART *dbg = new CBoardUART(BOARD_UART_DEBUG,rate,true,true,OnUARTRecvByte,(void*)&debug2device);
  CDebugger::Init();

  CBoardButton btn(BOARD_BUTTON1);
  //new CBoardButton(BOARD_BUTTON2,OnButton,new CBoardLED(BOARD_LED2));
  //new CBoardButton(BOARD_BUTTON3,OnButton,new CBoardLED(BOARD_LED3));


  if ( !CRTC::Init(16,12,31,RTC_Weekday_Saturday,23,59,05) )
     {
       CBoardLED(BOARD_LED1).On();
       CBoardLED(BOARD_LED2).On();
       CBoardLED(BOARD_LED3).On();
       CBoardLED(BOARD_LED4).On();
       CSysTicks::DelayInfinite();
     }


  CBoardLED led1(BOARD_LED1);
  CBoardLED led2(BOARD_LED2);
  CBoardLED led3(BOARD_LED3);
  CBoardLED led4(BOARD_LED4);

  while(1)
  {
    while (1)
    {
      bool init_ok = CSDCard::InitCard();

      led4.SetState(!init_ok);
      led1.SetState(init_ok);

      if ( init_ok )
         {
           printf("%lld\n",CSDCard::GetCardCapacity());
           break;
         }

      CSysTicks::Delay(2000);
    }

    printf("--- test begin ---\n");
    //TestSectorRW(29296870);
    //TestSectorRW(29296875*2);
    //TestSectorRW(29296825);
    //TestSectorR(1234);
    //TestSectorR(1239);
    //TestSectorR(1239);
    //TestSectorRW(29296870);
    //TestSectorR(29296870);
    //TestSectorR(29296870);
    //TestSectorR(29296870*2);
    //TestSectorR(1249);
    //TestSectorRW(2249);

    FATFS ffs;
    CLEAROBJ(ffs);
    f_mount(&ffs,"0:",1);  // should always succeed in our case
    
    {
      CLog log("new_log.txt",true);

      log.Add("Hello %f",123.456);
      CSysTicks::Delay(1000);
      log.Add("Hello %f",321.111);
    }

    
    printf("--- test end ---\n");
    
    while (1){}
  }


/*  while (1)
  {
    if ( !btn.IsDown() )
       {
         char yy,mm,dd,wd,hh,nn,ss;
         int subs;

         if ( CRTC::GetTS(yy,mm,dd,wd,hh,nn,ss,subs) )
            {
              printf("TS Event(!): 20%02d-%02d-%02d(%d) %02d:%02d:%02d.%03d\n",yy,mm,dd,wd,hh,nn,ss,CRTC::SS2MS(subs));
              led2.On();
            }

         CRTC::GetTime(yy,mm,dd,wd,hh,nn,ss,subs);

         OURTIME ot = ConvertOurTime(yy+2000,mm,dd,hh,nn,ss,CRTC::SS2MS(subs));
         char s[100];
         OurTimeToString(ot,s);
         printf("             %s [%4d]\n",s,subs);
       }

    CSysTicks::Delay(100);
    led1.Toggle();
  }*/
}
