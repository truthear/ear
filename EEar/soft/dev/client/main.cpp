
#include "include.h"
#include <string>
#include <vector>




struct {
 CCriticalSection o_cs;
 double lat,lon;
 short gnss;
 bool is_synced;
 unsigned last_sync_time;
} gps_data;


unsigned last_ts_time = 0;
OURTIME last_ts_value = 0;  // TS value, not shifted



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


volatile bool mic_alert = false;
volatile unsigned last_alert_time = 0;


void MicCB(void *led,const int16_t* buff,int num_samples)
{
  int sum = 0;

  for ( int n = 0; n < num_samples; n++ )
      {
        short s = buff[n];
        if ( s < 0 )
         s = -s;
        sum += s;
      }

  sum /= num_samples;

  bool alert = sum>20000;

  if ( alert )
     {
       if ( CSysTicks::GetCounter() - last_alert_time > 2000 )
          {
            last_alert_time = CSysTicks::GetCounter();
            mic_alert = true;
          }
     }
  
  reinterpret_cast<CLED*>(led)->SetState(alert);
}



void PrintTopOfHeap()
{
  void *p = malloc(1);
  printf("%p\n",p);
  free(p);
}


int main()
{
  CLED *led4 = new CBoardLED(BOARD_LED4);
  CLED *led3 = new CBoardLED(BOARD_LED3);
  CLED *led2 = new CBoardLED(BOARD_LED2);
  CLED *led1 = new CBoardLED(BOARD_LED1);


  
  CCPUTicks::Init();
  CSysTicks::Init();
  CDebugger::Init();


  if ( !CRTC::Init(20,2,29,RTC_Weekday_Tuesday,23,59,50,OnTS,led4) )
     {
       led1->On();
       led2->On();
       led3->On();
       led4->On();
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

  CMic::Init(MicCB,led3);

  bool old_synced = false;

  bool init_sdcard_ok = CSDCard::InitCard();

  FATFS ffs;
  CLEAROBJ(ffs);
  f_mount(&ffs,"0:",1);  // should always succeed in our case

  CLog log("alerts.txt",true);
  log.Add("--- System started ---");

  log.Add("SDCard Init %s",init_sdcard_ok?"OK":"failed");

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

    led1->SetState(is_synced);

    if ( mic_alert )
       {
         mic_alert = false;
         unsigned ev_time = last_alert_time;

         if ( is_synced )
            {
              OURTIME true_time = last_ts_value + (ev_time - last_ts_time) + CRTC::GetShift();

              char s[100];
              COurTime(true_time).ToString(s);

              log.Add("Event at %s !!!",s);
            }
         else
            {
              log.Add("Alert, but no time sync");
            }

         led2->On();
         CCPUTicks::Delay(100);
         led2->Off();
       }

    //unsigned ref_sec = (unsigned)(CRTC::GetTime()/1000);
    //while ( (unsigned)(CRTC::GetTime()/1000) == ref_sec ) {}
    //log.Add("(%s) %u %u %u, %c%c: %.6f %.6f",is_synced?"!":" ",mic_irq,delta_ticks_sys,delta_ticks_cpu,(gnss>>8)&0xFF,gnss&0xFF,lat,lon);
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
