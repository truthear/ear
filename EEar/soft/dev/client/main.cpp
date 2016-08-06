
#include "include.h"
//#include <vector>


/*
static const int MAX_SAMPLES = CMic::SAMPLE_RATE*2;  // 2 sec

short voice[MAX_SAMPLES];
volatile int voice_pos = 0;


void MicCB(void *led,const int16_t* buff,int num_samples)
{
  int sum = 0;

  int dest_pos = voice_pos;
  
  for ( int n = 0; n < num_samples; n++ )
      {
        short s = buff[n];

        voice[dest_pos+n] = s;
        
        if ( s < 0 )
         s = -s;

        sum += s;
      }

  dest_pos += num_samples;
  if ( dest_pos == MAX_SAMPLES )
   dest_pos = 0;
  voice_pos = dest_pos;
      
  sum /= num_samples;
  
  reinterpret_cast<CLED*>(led)->SetState(sum>1000);
}
*/


static const unsigned NMEA_AR_MAX = 16;
static char nmeas[NMEA_AR_MAX][128];
static volatile unsigned curr_nmea = 0;


void OnGPS(void *parm,OURTIME _time,double _lat,double _lon,short _gnss)
{
  unsigned idx = curr_nmea;

  char s[100];
  sprintf(nmeas[idx],"%c%c: [%s], %.6f %.6f",(_gnss>>8)&0xFF,_gnss&0xFF,OurTimeToString(_time+3*3600*1000,s),_lat,_lon);

  idx++;
  if ( idx == NMEA_AR_MAX )
   idx = 0;

  curr_nmea = idx;
}


int main()
{
  CCPUTicks::Init();
  CSysTicks::Init();
  CDebugger::Init();

  CBoardGPS *gps = new CBoardGPS(115200,OnGPS,NULL);

  CSysTicks::Delay(100);
  gps->EnableOnlyRMC();
  gps->SetSearchMode(true/*GPS*/,false/*!!!GLONASS*/,false,false);

  unsigned read_pos = 0;

  while (1)
  {
    unsigned write_pos = curr_nmea;

    while ( read_pos != write_pos )
    {
      printf("%s\n",nmeas[read_pos]);

      read_pos++;
      if ( read_pos == NMEA_AR_MAX )
       read_pos = 0;
    }
  }


//  if ( !CRTC::Init(16,12,31,RTC_Weekday_Saturday,23,59,05) )
//     {
//       CBoardLED(BOARD_LED1).On();
//       CBoardLED(BOARD_LED2).On();
//       CBoardLED(BOARD_LED3).On();
//       CBoardLED(BOARD_LED4).On();
//       CSysTicks::DelayInfinite();
//     }


/*  CBoardLED led1(BOARD_LED1);
  CBoardLED led2(BOARD_LED2);
  CBoardLED led3(BOARD_LED3);
  CBoardLED led4(BOARD_LED4);

  while (1)
  {
    bool init_ok = CSDCard::InitCard();

    led4.SetState(!init_ok);
    led1.SetState(init_ok);

    if ( init_ok )
      break;

    CSysTicks::Delay(2000);
  }

  printf("--- test begin ---\n");

  FATFS ffs;
  CLEAROBJ(ffs);
  f_mount(&ffs,"0:",1);  // should always succeed in our case

  FIL f;
  f_open(&f,"voice.raw",FA_CREATE_ALWAYS|FA_WRITE);

  CMic::Init(MicCB,&led3);

  int last_part = 1;

  while(!btn.IsDown()) 
  {
    int safe_part = voice_pos < MAX_SAMPLES/2 ? 1 : 0;
    if ( safe_part != last_part )
       {
         UINT wb = 0;
         f_write(&f,voice+safe_part*(MAX_SAMPLES/2),(MAX_SAMPLES/2)*sizeof(short),&wb);

         last_part = safe_part;
       }
  }

  f_close(&f);

  printf("--- test end ---\n");

  while(1) {}
*/

}
