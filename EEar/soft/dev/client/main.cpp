
#include "include.h"



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

  //reinterpret_cast<CLED*>(led)->Toggle();
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


//bool mic_alert = false;
//unsigned last_alert_time = 0;

//static const unsigned MAX_MIC_SUMS = 50;
//int mic_sums[MAX_MIC_SUMS];
//unsigned mic_sums_widx = 0;
//volatile unsigned mic_cb_counter = 0;


void MicCB(void *led,const int16_t* buff,int num_samples)
{
//  mic_cb_counter++;

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



//void PrintTopOfHeap()
//{
//  void *p = malloc(1);
//  printf("%p\n",p);
//  free(p);
//}



void OnUSSD(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( !is_timeout )
     {
       if ( !is_answered_ok )
        printf("USSD: failed\n");
       else
        printf("USSD: \"%s\"\n",CTelitMobile::DecodeUSSDAnswer(answer).c_str());
     }
  else
   printf("USSD: timeout\n");
}


void OnSMS(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( !is_timeout )
     {
       if ( !is_answered_ok )
        printf("SMS: failed \"%s\"\n",answer);
       else
        printf("SMS: OK\n");
     }
  else
   printf("SMS: timeout\n");
}


void OnSendString(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( !is_timeout )
     {
       if ( !is_answered_ok )
        printf("Send: failed \"%s\"\n",answer);
       else
        printf("Send: OK\n");
     }
  else
   printf("Send: timeout\n");
}



//void OnButton(void*parm)
//{
//  reinterpret_cast<CUART*>(parm)->SendByte(5);
//
//  last_alert_time = mic_cb_counter; //CSysTicks::GetCounter();
//  mic_alert = true;
//}
//
//
//void OnUart(void*,unsigned char data)
//{
//  if ( data == 5 )
//     {
//       last_alert_time = mic_cb_counter; //CSysTicks::GetCounter();
//       mic_alert = true;
//     }
//}

TCFG m_cfg;


// returns Base64 string of [CRC32+AES]
std::string EncodePacket(const void *sbuff,unsigned ssize)
{
  std::string rc;
  
  if ( ssize > 0 )
     {
       unsigned numblocks = (ssize+15)/16;
       unsigned align_size = numblocks*16;
       unsigned total_size = sizeof(unsigned)+align_size; // CRC32+16_aligned_packet

       char *p = (char*)alloca(total_size);  // not need to free

       // copy src packet
       memcpy(p+sizeof(unsigned),sbuff,ssize);

       // calc and save CRC32 of [packet+align_shit]:
       *(unsigned*)p = CRC32(0,(const unsigned char*)(p+sizeof(unsigned)),align_size);  

       // encode aligned data:
       AESCONTEXT ctx;
       aes_setkey_enc(&ctx,(const unsigned char*)OUR_AES_KEY,OUR_AES_KEY_SIZE);
       for ( unsigned n = 0; n < numblocks; n++ )
           {
             unsigned char *ioblock = (unsigned char*)(p+sizeof(unsigned)+n*16);
             aes_crypt_ecb_enc(&ctx,ioblock,ioblock);
           }

       // base64 of all data:
       rc = Base64Encode(p,total_size);
     }

  return rc;
}


std::string PreparePingPacket()
{
  TCmdServerPing i;

  i.header.cmd_id = CMDID_SERVER_PING;
  i.header.sector = m_cfg.sector;
  i.header.device_id = m_cfg.device_id;
  i.header.client_ver = CLIENT_VER;
  i.header.fdetect_ver = FDETECT_VER;
  i.time_utc = CRTC::GetTime();
  i.geo.lat = GEO2INT(gps_data.lat);
  i.geo.lon = GEO2INT(gps_data.lon);
  i.last_timesync_ms = CSysTicks::GetCounter()-gps_data.last_sync_time;

  return EncodePacket(&i,sizeof(i));
}



int main()
{
//  CLEAROBJ(mic_sums);
  
  CCPUTicks::Init();
  CSysTicks::Init();
  CDebugger::Init();

  ReadConfig(m_cfg);

//  CUART *uart = new CBoardUART(BOARD_UART_DEBUG,115200,true,true,OnUart);

  CLED *led4 = new CBoardLED(BOARD_LED4);
  CLED *led3 = new CBoardLED(BOARD_LED3);
  CLED *led2 = new CBoardLED(BOARD_LED2);
  CLED *led1 = new CBoardLED(BOARD_LED1);

  CBoardButton btn1(BOARD_BUTTON1);
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

//  bool init_sdcard_ok = CSDCard::InitCard();

//  FATFS ffs;
//  CLEAROBJ(ffs);
//  f_mount(&ffs,"0:",1);  // should always succeed in our case

  plog = new CLog(NULL/*"log.txt"*/,true);
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

  CTelitMobile *term = new CTelitMobile();

  if ( !term->Startup() )
     {
       led1->On();
       led2->On();
       led3->On();
       led4->On();
       CSysTicks::DelayInfinite();
     }

  unsigned last_term_update_time = CSysTicks::GetCounter() - 10000;
  unsigned last_send_time = CSysTicks::GetCounter();
  unsigned last_display_time = CSysTicks::GetCounter();

//  CMic::Init(MicCB,led1);

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
              gps->SetSearchMode(true/*GPS*/,true/*GLONASS*/,false,false);
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

    term->Poll();
    
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

    //{
    //  char id = is_synced ? (char)(gnss&0xFF) : '?';
    //  led2->SetState(id=='N'||id=='P');
    //  led3->SetState(id=='N'||id=='L');
    //}

    if ( CSysTicks::GetCounter() - last_term_update_time > 2000 )
       {
         term->UpdateSIMStatus();
         term->UpdateNetStatus();
         term->UpdateGPRSStatus();
         term->UpdateSignalQuality();
         term->UpdateInternetConnectionStatus();

         last_term_update_time = CSysTicks::GetCounter();
       }

    led1->SetState(term->GetSIMStatus()==SIM_READY);
    led2->SetState(term->GetNetStatus()==NET_REGISTERED_HOME);
    led3->SetState(term->GetGPRSStatus()==NET_REGISTERED_HOME);
    led4->SetState(term->GetInternetConnectionStatus());

    if ( btn1.IsDown() )
       {
         CSysTicks::Delay(1000);
         term->InitiateInternetConnection("internet");
       }

    if ( btn2.IsDown() )
       {
         CSysTicks::Delay(1000);
         std::string packet = PreparePingPacket();
         term->SendStringTCP(m_cfg.server.c_str(),m_cfg.port_tcp,packet.c_str(),OnSendString);
         printf("TCP: %s\n",packet.c_str());
       }

    if ( btn3.IsDown() )
       {
         CSysTicks::Delay(1000);
         std::string packet = PreparePingPacket();
         term->SendStringUDP(m_cfg.server.c_str(),m_cfg.port_udp,packet.c_str(),OnSendString);
         printf("UDP: %s\n",packet.c_str());
         //term->InitUSSDRequest("*111#",OnUSSD);
       }

    if ( CSysTicks::GetCounter() - last_send_time > 30000 )
       {

         last_send_time = CSysTicks::GetCounter();
       }


    if ( CSysTicks::GetCounter() - last_display_time > 1000 )
       {
         plog->Add("SIM: %d, Net: %d, GPRS: %d, Inet: %d, Line: %d",term->GetSIMStatus(),
                                                                    term->GetNetStatus(),
                                                                    term->GetGPRSStatus(),
                                                                    (int)term->GetInternetConnectionStatus(),
                                                                    term->GetSignalQuality());

         last_display_time = CSysTicks::GetCounter();
       }



   // if ( mic_alert )
   //    {
   //      mic_alert = false;
   //      unsigned ev_time = last_alert_time;
   //
   //      plog->Add("Alert at mic counter: %u",ev_time);
   //      
   //      /*if ( is_synced )
   //         {
   //           OURTIME true_time = last_ts_value + (ev_time - last_ts_time) + CRTC::GetShift();
   //
   //           char s[100];
   //           COurTime(true_time).ToString(s);
   //
   //           plog->Add("Event at %s (%c%c) %.7f %.7f !!!",s,(gnss>>8)&0xFF,gnss&0xFF,lat,lon);
   //         }
   //      else
   //         {
   //           plog->Add("Alert, but no time sync");
   //         }*/
   //
   //      led1->On();
   //      led4->On();
   //      CCPUTicks::Delay(100);
   //      led1->Off();
   //      led4->Off();
   //    }

  }


}
