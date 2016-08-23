
#include "include.h"



CLog *plog = NULL;
TCFG m_cfg;

CLED *led4 = NULL;
CLED *led3 = NULL;
CLED *led2 = NULL;
CLED *led1 = NULL;



void OnTS(void *sat,OURTIME ts,OURTIME)
{
  reinterpret_cast<CSatellite*>(sat)->OnTS(ts);
//  led4->Toggle();
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


std::string PreparePingPacket(double lat,double lon,unsigned last_time_sync)
{
  TCmdServerPing i;

  i.header.cmd_id = CMDID_SERVER_PING;
  i.header.sector = m_cfg.sector;
  i.header.device_id = m_cfg.device_id;
  i.header.client_ver = CLIENT_VER;
  i.header.fdetect_ver = FDETECT_VER;
  i.time_utc = CRTC::GetTime();
  i.geo.lat = GEO2INT(lat);
  i.geo.lon = GEO2INT(lon);
  i.last_timesync_ms = CSysTicks::GetCounter()-last_time_sync;

  return EncodePacket(&i,sizeof(i));
}


std::string PrepareUSSDPacket(const char *text)
{
  text = NNS(text);

  unsigned size = sizeof(TCmdHeader)+strlen(text)+1;
  
  TCmdUSSDBalance *i = (TCmdUSSDBalance*)alloca(size);  // not need to free

  i->header.cmd_id = CMDID_USSD_BALANCE;
  i->header.sector = m_cfg.sector;
  i->header.device_id = m_cfg.device_id;
  i->header.client_ver = CLIENT_VER;
  i->header.fdetect_ver = FDETECT_VER;
  strcpy(i->text,text);

  return EncodePacket(i,size);
}


std::string PrepareFDetectPacket(OURTIME event_time,double lat,double lon)
{
  TCmdFDetect i;

  i.header.cmd_id = CMDID_FDETECT;
  i.header.sector = m_cfg.sector;
  i.header.device_id = m_cfg.device_id;
  i.header.client_ver = CLIENT_VER;
  i.header.fdetect_ver = FDETECT_VER;
  i.time_utc = event_time;
  i.geo.lat = GEO2INT(lat);
  i.geo.lon = GEO2INT(lon);

  return EncodePacket(&i,sizeof(i));
}




void OnSMS(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( !is_timeout )
     {
       if ( !is_answered_ok )
        plog->Add("SMS: failed \"%s\"\n",answer);
       else
        plog->Add("SMS: OK\n");
     }
  else
   plog->Add("SMS: timeout\n");
}


void OnSendString(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( !is_timeout )
     {
       if ( !is_answered_ok )
        plog->Add("Send: failed \"%s\"\n",answer);
       else
        plog->Add("Send: OK\n");
     }
  else
   plog->Add("Send: timeout\n");
}

void OnUSSD(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( !is_timeout )
     {
       if ( !is_answered_ok )
        plog->Add("USSD: failed\n");
       else
       {
         std::string ussd = CTelitMobile::DecodeUSSDAnswer(answer);
         
         plog->Add("USSD: \"%s\"\n",ussd.c_str());

         CTelitMobile *term = reinterpret_cast<CTelitMobile*>(parm);

         std::string packet = PrepareUSSDPacket(ussd.c_str());

         term->SendStringUDP(m_cfg.server.c_str(),m_cfg.port_udp,packet.c_str(),OnSendString);
         plog->Add("UDP: %s\n",packet.c_str());
      }
     }
  else
   plog->Add("USSD: timeout\n");
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




int main()
{
//  CLEAROBJ(mic_sums);


  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);  // 4 bits for preemption priority, 0 for subpriority
  
  CCPUTicks::Init();
  CSysTicks::Init();
  CDebugger::Init();

  led4 = new CBoardLED(BOARD_LED4);
  led3 = new CBoardLED(BOARD_LED3);
  led2 = new CBoardLED(BOARD_LED2);
  led1 = new CBoardLED(BOARD_LED1);

  CBoardButton btn1(BOARD_BUTTON1);
  CBoardButton btn2(BOARD_BUTTON2);
  CBoardButton btn3(BOARD_BUTTON3);

  CSatellite *sat = new CSatellite(115200,true/*GPS*/,true/*Glonass*/,false,false);

  if ( !CRTC::Init(20,2,29,RTC_Weekday_Tuesday,23,59,50,OnTS,sat) )
     {
       led1->On();
       led2->On();
       led3->On();
       led4->On();
       CSysTicks::DelayInfinite();
     }

  if ( !CSDCard::InitCard() )
     {
       led1->On();
       led2->On();
       led3->On();
       led4->On();
       CSysTicks::DelayInfinite();
     }

  FATFS ffs;
  CLEAROBJ(ffs);
  f_mount(&ffs,"0:",1);  // should always succeed in our case

  if ( !ReadConfig(m_cfg) )
     {
       led4->On();
       CSysTicks::DelayInfinite();
     }


  plog = new CLog(NULL/*"log3.txt"*/,true);
  plog->Add("--- System started ---");

  bool old_synced = false;

  CTelitMobile *term = new CTelitMobile(115200,100);

  if ( !term->Startup() )
     {
       led1->On();
       led2->On();
       led3->On();
       led4->On();
       CSysTicks::DelayInfinite();
     }

  unsigned last_term_update_time = CSysTicks::GetCounter() - 10000;
  //unsigned last_send_time = CSysTicks::GetCounter();
  unsigned last_display_time = CSysTicks::GetCounter();

//  CMic::Init(MicCB,led1);

//  unsigned old_mic_safe_read = 1;
//
//  CLEAROBJ(samples);

//  FIL voicef;
//  f_open(&voicef,"mic.raw",FA_WRITE|FA_OPEN_APPEND);



  while (1)
  {
    
    
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

    sat->Poll();
    term->Poll();
    
    double lat=0,lon=0;
    short gnss=0x3f3f;
    if ( !sat->GetNavData(lat,lon,gnss) )
       {
         gnss = 0x3f3f;
       }

    OURTIME shift = 0;
    bool is_synced = sat->GetTimeData(shift);
    if ( is_synced )
       {
         CRTC::SetShift(shift);
       }

    if ( old_synced != is_synced )
       {
         if ( is_synced )
           plog->Add("Time sync event !!!");
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
         //std::string packet = PreparePingPacket(lat,lon,sat->GetLastSyncTime());
         std::string packet = PrepareFDetectPacket(CRTC::GetTime(),lat,lon);
         term->SendStringTCP(m_cfg.server.c_str(),m_cfg.port_tcp,packet.c_str(),OnSendString);
         //plog->Add("TCP: %s\n",packet.c_str());
       }

    if ( btn3.IsDown() )
       {
         //led1->Off();
         //led2->Off();
         //led3->Off();
         //led4->Off();
         
         CSysTicks::Delay(1000);
         std::string packet = PreparePingPacket(lat,lon,sat->GetLastSyncTime());
         term->SendStringUDP(m_cfg.server.c_str(),m_cfg.port_udp,packet.c_str(),OnSendString);
         //plog->Add("UDP: %s\n",packet.c_str());
         //term->InitUSSDRequest(m_cfg.ussd_balance.c_str(),OnUSSD,term);
         //plog->Add("USSD initiated...\n");
       }

    //if ( CSysTicks::GetCounter() - last_send_time > 30000 )
    //   {
    //
    //     last_send_time = CSysTicks::GetCounter();
    //   }


    if ( CSysTicks::GetCounter() - last_display_time > 1000 )
       {
         plog->Add("SIM:%d, Net:%d, GPRS:%d, Inet:%d, Line:%2d, GNSS:%c%c",term->GetSIMStatus(),
                                                                    term->GetNetStatus(),
                                                                    term->GetGPRSStatus(),
                                                                    (int)term->GetInternetConnectionStatus(),
                                                                    term->GetSignalQuality(),
                                                                    (gnss>>8)&0xFF,gnss&0xFF);

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
