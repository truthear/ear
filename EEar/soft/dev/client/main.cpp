
#include "include.h"
#include <vector>
#include <math.h>


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


//static const unsigned NMEA_AR_MAX = 16;
//static char nmeas[NMEA_AR_MAX][128];
//static volatile unsigned curr_nmea = 0;



struct {
 CCriticalSection o_cs;
 double lat,lon;
 short gnss;
 bool is_synced;
 unsigned last_sync_time;
} gps_data;


unsigned last_ts_time = 0;  // no CS needed
OURTIME last_ts_value = 0;


void OnTS(void *led,OURTIME ts,OURTIME)
{
  last_ts_time = CSysTicks::GetCounter();
  last_ts_value = ts;

  reinterpret_cast<CLED*>(led)->Toggle();
}


void OnGPS(void *parm,OURTIME _time,double _lat,double _lon,short _gnss)
{
  if ( (_time % 1000) == 0 )  // only PPS time needed
     {
       bool synced = false;
       
       if ( CSysTicks::GetCounter() - last_ts_time < 1000 )
          {
            CRTC::SetShift(_time - last_ts_value);
            synced = true;
          }

       if ( gps_data.o_cs.IsUnlocked() )
          {
            gps_data.lat = _lat;
            gps_data.lon = _lon;
            gps_data.gnss = _gnss;

            if ( synced )
               {
                 gps_data.is_synced = true;
                 gps_data.last_sync_time = CSysTicks::GetCounter();
               }
          }
     }
}



int main()
{
  CLED *led = new CBoardLED(BOARD_LED4);
  
  CCPUTicks::Init();
  CSysTicks::Init();
  CDebugger::Init();


  if ( !CRTC::Init(19,1,1,RTC_Weekday_Tuesday,0,0,0,OnTS,led) )
     {
       CBoardLED(BOARD_LED1).On();
       CBoardLED(BOARD_LED2).On();
       CBoardLED(BOARD_LED3).On();
       CBoardLED(BOARD_LED4).On();
       CSysTicks::DelayInfinite();
     }



  gps_data.lat = 0;
  gps_data.lon = 0;
  gps_data.gnss = 0x3f3f;
  gps_data.is_synced = false;
  gps_data.last_sync_time = 0;

  CBoardGPS *gps = new CBoardGPS(115200,OnGPS,NULL);

  CSysTicks::Delay(100);
  gps->EnableOnlyRMC();
  gps->SetSearchMode(true/*GPS*/,true/*GLONASS*/,false,false);


  bool old_synced = false;

  CLog log(NULL,true);
  log.Add("--- System started ---");

  while (1)
  {
    double lat,lon;
    short gnss;
    bool is_synced;
    
    {
      CCSGuard g(gps_data.o_cs);

      lat = gps_data.lat;
      lon = gps_data.lon;
      gnss = gps_data.gnss;

      if ( gps_data.is_synced )
         {
           if ( CSysTicks::GetCounter() - gps_data.last_sync_time > 30000 )
              {
                gps_data.is_synced = false;
              }
         }
      
      is_synced = gps_data.is_synced;
    }

    if ( old_synced != is_synced )
       {
         if ( is_synced )
           log.Add("Time sync event !!!");
         else
           log.Add("We\'ve lost time sync...");

         old_synced = is_synced;
       }

    unsigned ref_sec = (unsigned)(CRTC::GetTime()/1000);
    while ( (unsigned)(CRTC::GetTime()/1000) == ref_sec ) {}
    
    log.Add("(%s) %lld, %c%c: %.6f %.6f",is_synced?"!":" ",CRTC::GetShift(),(gnss>>8)&0xFF,gnss&0xFF,lat,lon);
  }


//  unsigned read_pos = 0;
//
//  while (1)
//  {
//    unsigned write_pos = curr_nmea;
//
//    while ( read_pos != write_pos )
//    {
//      printf("%s\n",nmeas[read_pos]);
//
//      read_pos++;
//      if ( read_pos == NMEA_AR_MAX )
//       read_pos = 0;
//    }
//  }




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
