
#include "include.h"
#include <string>
#include <vector>



CLog *plog = NULL;


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


//static const unsigned BUFFSAMPLES = CMic::SAMPLE_RATE*2;  // 2 sec
//short samples[BUFFSAMPLES];
//volatile unsigned samples_wto = 0;
//bool mic_pause = false;


bool mic_alert = false;
unsigned last_alert_time = 0;

static const unsigned MAX_MIC_SUMS = 50;
int mic_sums[MAX_MIC_SUMS];
unsigned mic_sums_widx = 0;
volatile unsigned mic_cb_counter = 0;


void MicCB(void *led,const int16_t* buff,int num_samples)
{
  mic_cb_counter++;

//  int sum = 0;

//  if ( !mic_pause )
/*     {
       for ( int n = 0; n < num_samples; n++ )
           {
             short s = buff[n];
             if ( s < 0 )
              s = -s;
             sum += s;
           }
  
       sum /= num_samples;

       mic_sums[mic_sums_widx++] = sum;
       if ( mic_sums_widx == MAX_MIC_SUMS )
          {
            mic_sums_widx = 0;
          }

       sum = 0;
       for ( unsigned n = 0; n < MAX_MIC_SUMS; n++ )
       {
         sum += mic_sums[n];
       }
       sum /= MAX_MIC_SUMS;
       
       bool alert = sum>25000;

       if ( alert )
          {
            if ( CSysTicks::GetCounter() - last_alert_time > 2000 )
               {
                 last_alert_time = CSysTicks::GetCounter();
                 mic_alert = true;
               }
          }
       
     }*/

//  reinterpret_cast<CLED*>(led)->SetState(sum>2000);
//
//  unsigned idx = samples_wto;
//
//  for ( int n = 0; n < num_samples; n++ )
//      {
//        samples[idx+n] = mic_pause ? 0 : buff[n];
//      }
//
//  idx += num_samples;
//  if ( idx == BUFFSAMPLES )
//    idx = 0;
//
//  samples_wto = idx;
}



void PrintTopOfHeap()
{
  void *p = malloc(1);
  printf("%p\n",p);
  free(p);
}

/*
void OnUSSD(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( !is_timeout )
     {
       if ( !is_answered_ok )
        printf("USSD: failed\n");
       else
        printf("USSD: %s\n",CMobile::DecodeUSSDAnswer(answer).c_str());
     }
  else
   printf("USSD: timeout\n");
}


void OnSMS(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( !is_timeout )
     {
       if ( !is_answered_ok )
        printf("SMS: failed\n");
       else
        printf("SMS: OK\n");
     }
  else
   printf("SMS: timeout\n");
}
*/

/*
void OnSendString(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( !is_timeout )
     {
       if ( !is_answered_ok )
        plog->Add("TCPSend: failed\n");
       else
        plog->Add("TCPSend: OK\n");
     }
  else
   plog->Add("TCPSend: timeout\n");

  mic_pause = false;
}
*/


void OnButton(void*parm)
{
  reinterpret_cast<CUART*>(parm)->SendByte(5);

  last_alert_time = mic_cb_counter; //CSysTicks::GetCounter();
  mic_alert = true;
}


void OnUart(void*,unsigned char data)
{
  if ( data == 5 )
     {
       last_alert_time = mic_cb_counter; //CSysTicks::GetCounter();
       mic_alert = true;
     }
}


int main()
{
  CLEAROBJ(mic_sums);

  
  CCPUTicks::Init();
  CSysTicks::Init();
  CDebugger::Init();


//  CUART *uart = new CBoardUART(BOARD_UART_DEBUG,115200,true,true,OnUart);

  CLED *led4 = new CBoardLED(BOARD_LED4);
  CLED *led3 = new CBoardLED(BOARD_LED3);
  CLED *led2 = new CBoardLED(BOARD_LED2);
  CLED *led1 = new CBoardLED(BOARD_LED1);

  CBoardButton btn1(BOARD_BUTTON1/*,OnButton,uart*/);
  CBoardButton btn2(BOARD_BUTTON2);
  CBoardButton btn3(BOARD_BUTTON3);

  if ( !CRTC::Init(20,2,29,RTC_Weekday_Tuesday,23,59,50,OnTS,led4) )
     {
       led1->On();
       led2->On();
       led3->On();
       led4->On();
       CSysTicks::DelayInfinite();
     }

  bool init_sdcard_ok = CSDCard::InitCard();

  FATFS ffs;
  CLEAROBJ(ffs);
  f_mount(&ffs,"0:",1);  // should always succeed in our case

  plog = new CLog("log.txt",false/*true*/);  //!!!!!!!!!!!!!!!!
  plog->Add("--- System started ---");

  gps_data.lat = 0;
  gps_data.lon = 0;
  gps_data.gnss = 0x3f3f;
  gps_data.is_synced = false;
  gps_data.last_sync_time = 0;

  CBoardGPS *gps = new CBoardGPS(115200,OnGPS,NULL);

  unsigned gps_starttime = CSysTicks::GetCounter();
  bool gps_initialized = false;

  bool old_synced = false;

//  CMobile *term = new CMobile(115200);
//
//  if ( !term->Startup() )
//     {
//       led1->On();
//       led2->On();
//       led3->On();
//       led4->On();
//       CSysTicks::DelayInfinite();
//     }

//  unsigned last_term_update_time = CSysTicks::GetCounter() - 10000;
//  unsigned last_send_time = CSysTicks::GetCounter();

  CMic::Init(MicCB,led1);

//  unsigned old_mic_safe_read = 1;
//
//  CLEAROBJ(samples);

//  FIL voicef;
//  f_open(&voicef,"mic.raw",FA_WRITE|FA_OPEN_APPEND);

  while (1)
  {
    if ( !gps_initialized )
       {
         if ( CSysTicks::GetCounter() - gps_starttime > gps->GetWarmingUpTime() )
            {
              gps->EnableOnlyRMC();

              //gps->SetSearchMode(true/*GPS*/,true/*GLONASS*/,false,false);

              gps->SetSearchMode(true/*GPS*/,false/*GLONASS*/,false,false);
              //gps->SetSearchMode(false/*GPS*/,true/*GLONASS*/,false,false);

              gps_initialized = true;
            }
       }
    
    
//    unsigned mic_safe_read = samples_wto >= BUFFSAMPLES/2 ? 0 : 1;
//    
//    if ( mic_safe_read != old_mic_safe_read )
//       {
//         UINT wb = 0;
//         f_write(&voicef,&samples[mic_safe_read*BUFFSAMPLES/2],BUFFSAMPLES/2*sizeof(short),&wb);
//
//         old_mic_safe_read = mic_safe_read;
//       }
    
//    if ( btn1.IsDown() )
//       {
//         f_close(&voicef);
//
//         led1->On();
//         led2->On();
//         led3->On();
//         led4->On();
//         CSysTicks::DelayInfinite();
//       }
//       
//    term->Poll();
    
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
           plog->Add("Time sync event !!! %c%c",(gnss>>8)&0xFF,gnss&0xFF);
         else
           plog->Add("We\'ve lost time sync...");

         old_synced = is_synced;
       }

    {
      char id = is_synced ? (char)(gnss&0xFF) : '?';
      led2->SetState(id=='N'||id=='P');
      led3->SetState(id=='N'||id=='L');
    }

//    if ( CSysTicks::GetCounter() - last_term_update_time > 2000 )
//       {
//         //term->UpdateSIMStatus();
//         term->UpdateNetStatus();
//         //term->UpdateGPRSStatus();
//         //term->UpdateSignalQuality();
//         //term->UpdateInternetConnectionStatus();
//
//         last_term_update_time = CSysTicks::GetCounter();
//       }
//
//    led3->SetState(term->GetNetStatus()==NET_REGISTERED_HOME);
//
//    if ( CSysTicks::GetCounter() - last_send_time > 30000 )
//       {
//         //mic_pause = true;
//         
//         term->InitiateInternetConnection("internet");
//         
//         char tim[100];
//         COurTime(CRTC::GetTime()).ToString(tim);
//
//         char s[100];
//         sprintf(s,"[%s] %.7f %.7f",tim,lat,lon);
//         term->SendStringTCP("195.234.5.137",81,s,OnSendString);
//         //term->SendSMS("+380935237031",s,OnSendString);
//
//         plog->Add("(%s) %c%c: %.7f %.7f",is_synced?"!":" ",(gnss>>8)&0xFF,gnss&0xFF,lat,lon);
//
//         last_send_time = CSysTicks::GetCounter();
//       }



    if ( mic_alert )
       {
         mic_alert = false;
         unsigned ev_time = last_alert_time;

         plog->Add("Alert at mic counter: %u",ev_time);
         
         /*if ( is_synced )
            {
              OURTIME true_time = last_ts_value + (ev_time - last_ts_time) + CRTC::GetShift();

              char s[100];
              COurTime(true_time).ToString(s);

              plog->Add("Event at %s (%c%c) %.7f %.7f !!!",s,(gnss>>8)&0xFF,gnss&0xFF,lat,lon);
            }
         else
            {
              plog->Add("Alert, but no time sync");
            }*/

         led1->On();
         led4->On();
         CCPUTicks::Delay(100);
         led1->Off();
         led4->Off();
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
