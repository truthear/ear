
#include "include.h"





CBoardLED* p_led1 = NULL;
CBoardLED* p_led2 = NULL;
CBoardLED* p_led3 = NULL;
CBoardLED* p_led4 = NULL;

void FatalError()
{
  __disable_irq();
  
  p_led1->On();
  p_led2->On();
  p_led3->On();
  p_led4->On();

  CTicksCommon::DelayInfinite();
}


void TestCluster(unsigned start)
{
  static const unsigned cluster = 8;
  static char buff[CSDCard::SECTOR_SIZE*cluster];
  static char buff2[CSDCard::SECTOR_SIZE*cluster];

  for ( unsigned m = 0; m < sizeof(buff); m++ )
  {
    buff[m] = rand();
  }

  bool is_timeout = false;
  bool rc = CSDCard::Write(buff,start*cluster,cluster,is_timeout);

  if ( !rc )
    printf("Error write\n");

     {
       rc = CSDCard::Read(buff2,start*cluster,cluster);

       if ( rc )
          {
            if ( memcmp(buff,buff2,sizeof(buff)) )
               {
                 printf("Cmp error at cluster %d (timeout: %s)\n",start,is_timeout?"TRUE":"false");
               }
            else
               {
                 if ( is_timeout )
                   printf("Timeout at cluster %d\n",start);
                 else
                 {
                   //printf("Cluster ok %d\n",start);
                 }
               }
          }
       else
          printf("Error read\n");
     }
}


int main()
{
  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);  // 4 bits for preemption priority, 0 for subpriority
  
  CCPUTicks::Init();
  CSysTicks::Init();

  CSysTicks::Delay(300);
  
  CDebugger::Init();
  // start from this point we can use printf()!

  srand(CCPUTicks::GetCounter());


  p_led1 = new CBoardLED(BOARD_LED1);
  p_led2 = new CBoardLED(BOARD_LED2);
  p_led3 = new CBoardLED(BOARD_LED3);
  p_led4 = new CBoardLED(BOARD_LED4);
  // start from this point we can use FatalError()

  CBoardButton *p_btn = new CBoardButton(BOARD_BUTTON2);

  p_led4->On();

  if ( !CSDCard::InitCard() )
     {
       printf("SDCard init failed\n");
       FatalError();
     }


  p_led4->Off();

  Beep(1000,50);

  printf("Capacity: %d MB\n",(int)(CSDCard::GetCardCapacity()/1024/1024));



//  TestCluster(0);
//  TestCluster(2000000);
//  TestCluster(40137);
//  TestCluster(40258);
//  TestCluster(1);

  for ( int n = 0; n < 5000; n++ )
   TestCluster(4000+n);
  



  p_led3->On();
  Beep(1000,1000);
}
















