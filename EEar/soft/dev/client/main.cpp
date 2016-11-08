
#include "include.h"



class CEar
{
          static const int GSM_MODEM_BAUDRATE = 115200;
          static const unsigned MOBILE_TERMINAL_MAX_QUEUE_COMMANDS = 25;
          static const bool MOBILE_TERMINAL_AUTO_ANSWER_MODE = true;
          static const bool LOG_STDOUT_ECHO = true;
          static const short INVALID_GNSS = 0x3f3f;  // '??'
          
          
          CBoardLED* m_leds[4];   // base objects
          CBoardLED* p_led1;      // pointer alias
          CBoardLED* p_led2;      // pointer alias
          CBoardLED* p_led3;      // pointer alias
          CBoardLED* p_led4;      // pointer alias
          CBoardLED* p_led_mic;   // pointer alias
          CBoardLED* p_led_gsm;   // pointer alias
          CBoardLED* p_led_sync;  // pointer alias
          CBoardLED* p_led_nosim; // pointer alias

          FATFS m_ffs;

          CConfig m_cfg;

          CSatellite *p_sat;
          CTelitMobile *p_mob;

          CLog *p_log;

          CButton *p_btn1;
          CButton *p_btn2;
          CButton *p_btn3;

          bool b_synced;  // is we are time syncronized with satellite? if true CRTC::GetTime() returns true UTC time
          double m_lat;   // initially set to 0
          double m_lon;   // initially set to 0
          short m_gnss;   // initially set to INVALID_GNSS

  public:
          CEar();
          ~CEar();

          void MainLoop();

  private:
          bool IsSynced() const { return b_synced; }
          bool IsGPSFix() const { return m_gnss != INVALID_GNSS; }
          
          void FatalError();
          static void IRQ_OnButton1Wrapper(void*);
          static void IRQ_OnButton2Wrapper(void*);
          static void IRQ_OnButton3Wrapper(void*);
          void IRQ_OnButton1();
          void IRQ_OnButton2();
          void IRQ_OnButton3();
          static void IRQ_OnTimeStamp(void*,OURTIME ts_unshifted,OURTIME ts_shifted);
          static void IRQ_OnMicWrapper(void*,const int16_t* pcm_buff,int num_samples);
          void IRQ_OnMic(const int16_t* pcm_buff,int num_samples);
          std::string EncodePacket(const void *sbuff,unsigned ssize);
          std::string PreparePingPacket();
          std::string PrepareUSSDPacket(const char *text);
          std::string PrepareFDetectPacket(OURTIME event_time);
          void UpdateSync(unsigned& last_update_time);
          void UpdateLatLon(unsigned& last_update_time);
          void UpdateGSMStatuses(unsigned& last_update_time,bool actual_update);

};



CEar::CEar()
{
  // clear variables
  CLEAROBJ(m_leds);
  p_led1 = NULL;
  p_led2 = NULL;
  p_led3 = NULL;
  p_led4 = NULL;
  p_led_mic = NULL;
  p_led_gsm = NULL;
  p_led_sync = NULL;
  p_led_nosim = NULL;
  p_sat = NULL;
  p_mob = NULL;
  p_log = NULL;
  p_btn1 = NULL;
  p_btn2 = NULL;
  p_btn3 = NULL;
  b_synced = false;
  m_lat = 0.0;
  m_lon = 0.0;
  m_gnss = INVALID_GNSS;


  // system init
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);  // 4 bits for preemption priority, 0 for subpriority
  
  CCPUTicks::Init();
  CSysTicks::Init();
  CDebugger::Init();
  // start from this point we can use printf()!

  m_leds[0] = new CBoardLED(BOARD_LED1);
  m_leds[1] = new CBoardLED(BOARD_LED2);
  m_leds[2] = new CBoardLED(BOARD_LED3);
  m_leds[3] = new CBoardLED(BOARD_LED4);
  p_led1 = m_leds[0];
  p_led2 = m_leds[1];
  p_led3 = m_leds[2];
  p_led4 = m_leds[3];
  p_led_mic = p_led1;
  p_led_gsm = p_led2;
  p_led_sync = p_led3;
  p_led_nosim = p_led4;
  // start from this point we can use FatalError()

  CSysTicks::Delay(100);  // paranoja

  if ( !CSDCard::InitCard() )
     {
       printf("SDCard init failed\n");
       FatalError();
     }

  // FatFs init
  CLEAROBJ(m_ffs);
  f_mount(&m_ffs,"0:",1);  // should always succeed in our case
  // start from this point we can use file i/o f_XXXX

  if ( !m_cfg.ReadConfig() )
     {
       printf("Read config failed\n");
       FatalError();
     }
  // start from this point we can use m_cfg.XXX

  p_sat = new CSatellite(m_cfg.gps_baud,m_cfg.use_gps,m_cfg.use_glonass,m_cfg.use_galileo,m_cfg.use_beidou);

  if ( !CRTC::Init(5,5,5,RTC_Weekday_Thursday,0,0,0,IRQ_OnTimeStamp,p_sat) )
     {
       printf("RTC init failed\n");
       FatalError();
     }
  // start from this point we can use CRTC::XXX

  p_mob = new CTelitMobile(GSM_MODEM_BAUDRATE,MOBILE_TERMINAL_MAX_QUEUE_COMMANDS);
  if ( !p_mob->Startup(MOBILE_TERMINAL_AUTO_ANSWER_MODE) )
     {
       printf("Mobile init failed\n");
       FatalError();
     }

  CMic::Init(IRQ_OnMicWrapper,this);

  p_log = new CLog(LOG_FILENAME,LOG_STDOUT_ECHO);
  // start from this point we can use ADD2LOG()

  p_btn1 = new CBoardButton(BOARD_BUTTON1,IRQ_OnButton1Wrapper,this);
  p_btn2 = new CBoardButton(BOARD_BUTTON2,IRQ_OnButton2Wrapper,this);
  p_btn3 = new CBoardButton(BOARD_BUTTON3,IRQ_OnButton3Wrapper,this);
}


CEar::~CEar()
{
  assert(false);
}


void CEar::FatalError()
{
  p_led1->On();
  p_led2->On();
  p_led3->On();
  p_led4->On();
  CTicksCommon::DelayInfinite();
}


// WARNING!!! This is an IRQ handler!
void CEar::IRQ_OnButton1Wrapper(void *parm)
{
  reinterpret_cast<CEar*>(parm)->IRQ_OnButton1();
}


// WARNING!!! This is an IRQ handler!
void CEar::IRQ_OnButton2Wrapper(void *parm)
{
  reinterpret_cast<CEar*>(parm)->IRQ_OnButton2();
}


// WARNING!!! This is an IRQ handler!
void CEar::IRQ_OnButton3Wrapper(void *parm)
{
  reinterpret_cast<CEar*>(parm)->IRQ_OnButton3();
}


// WARNING!!! This is an IRQ handler!
void CEar::IRQ_OnButton1()
{
}


// WARNING!!! This is an IRQ handler!
void CEar::IRQ_OnButton2()
{
}


// WARNING!!! This is an IRQ handler!
void CEar::IRQ_OnButton3()
{
}


// WARNING!!! This is an IRQ handler!
void CEar::IRQ_OnTimeStamp(void *parm,OURTIME ts_unshifted,OURTIME ts_shifted)
{
  reinterpret_cast<CSatellite*>(parm)->OnTS(ts_unshifted);
}


// WARNING!!! This is an IRQ handler!
void CEar::IRQ_OnMicWrapper(void *parm,const int16_t* pcm_buff,int num_samples)
{
  reinterpret_cast<CEar*>(parm)->IRQ_OnMic(pcm_buff,num_samples);
}


// WARNING!!! This is an IRQ handler!
void CEar::IRQ_OnMic(const int16_t* pcm_buff,int num_samples)
{
  int sum = 0;

  for ( int n = 0; n < num_samples; n++ )
      {
        short s = pcm_buff[n];
        if ( s < 0 )
         s = -s;
        sum += s;
      }

  sum /= num_samples;

  p_led_mic->SetState(sum>2000);
}


// returns Base64 string of [CRC32+AES]
std::string CEar::EncodePacket(const void *sbuff,unsigned ssize)
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


std::string CEar::PreparePingPacket()
{
  TCmdServerPing i;

  i.header.cmd_id = CMDID_SERVER_PING;
  i.header.sector = m_cfg.sector;
  i.header.device_id = m_cfg.device_id;
  i.header.client_ver = CLIENT_VER;
  i.header.fdetect_ver = FDETECT_VER;
  i.time_utc = CRTC::GetTime();
  i.geo.lat = GEO2INT(m_lat);
  i.geo.lon = GEO2INT(m_lon);
  i.last_timesync_ms = GetTickCount()-p_sat->GetLastSyncTime();

  return EncodePacket(&i,sizeof(i));
}


std::string CEar::PrepareUSSDPacket(const char *text)
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


std::string CEar::PrepareFDetectPacket(OURTIME event_time)
{
  TCmdFDetect i;

  i.header.cmd_id = CMDID_FDETECT;
  i.header.sector = m_cfg.sector;
  i.header.device_id = m_cfg.device_id;
  i.header.client_ver = CLIENT_VER;
  i.header.fdetect_ver = FDETECT_VER;
  i.time_utc = event_time;
  i.geo.lat = GEO2INT(m_lat);
  i.geo.lon = GEO2INT(m_lon);

  return EncodePacket(&i,sizeof(i));
}


void CEar::UpdateSync(unsigned& last_update_time)
{    
  if ( GetTickCount() - last_update_time > 1000 )
     {
       OURTIME shift = 0;
       bool sync = p_sat->GetTimeData(shift);
       if ( sync )
          {
            CRTC::SetShift(shift);  // update shift every second!
          }

       p_led_sync->SetState(sync);
          
       if ( !IsBoolEqu(sync,b_synced) )
          {
            b_synced = sync;

            ADD2LOG(("Sync: %s",sync?"YES":"no"));
          }

       last_update_time = GetTickCount();
     }
}


void CEar::UpdateLatLon(unsigned& last_update_time)
{
  if ( GetTickCount() - last_update_time > 1000 )
     {
       double lat,lon;
       short gnss;
       if ( p_sat->GetNavData(lat,lon,gnss) )
          {
            m_lat = lat;
            m_lon = lon;
            m_gnss = gnss;
          }

       last_update_time = GetTickCount();
     }
}


void CEar::UpdateGSMStatuses(unsigned& last_update_time,bool actual_update)
{
  if ( GetTickCount() - last_update_time > 5000 )
     {
       if ( actual_update )
          {
            p_mob->UpdateSIMStatus();
            p_mob->UpdateNetStatus();
            //p_mob->UpdateGPRSStatus();     // used for debug log
            //p_mob->UpdateSignalQuality();  // used for debug log
            //p_mob->UpdateInternetConnectionStatus();  // used for debug log
          }

       last_update_time = GetTickCount();
     }

  p_led_nosim->SetState(p_mob->GetSIMStatus()!=SIM_READY);
  p_led_gsm->SetState(p_mob->GetNetStatus()==NET_REGISTERED_HOME);
}



void CEar::MainLoop()
{
  ADD2LOG(("--- System started ---"));
  ADD2LOG(("device_id: %d, sector: %d",m_cfg.device_id,m_cfg.sector));
  ADD2LOG(("GPS:%d, Glonass:%d, Galileo:%d, Beidou:%d",m_cfg.use_gps,m_cfg.use_glonass,m_cfg.use_galileo,m_cfg.use_beidou));


  unsigned last_update_sync = GetTickCount();
  unsigned last_update_latlon = GetTickCount();
  unsigned last_update_gsm = GetTickCount()-5000;

  while ( 1 )
  {
    UpdateSync(last_update_sync);
    UpdateLatLon(last_update_latlon);
    UpdateGSMStatuses(last_update_gsm,true); // WARNING!!! Здесь может быть ситуация когда долго-выполняемая команда типа USSD/SMS/TCP/UDP висит на выполнении и мы вызываем периодически этот Update(true), то фактически только добавляем новые AT-команды в очередь и забиваем ее, что плохо. Решение: делать Update(false) пока выполняется какая-то долгая команда.



    // poll devices
    p_sat->Poll();
    p_mob->Poll();
  }
}



//////////////////


int main()
{
  (new CEar())->MainLoop();   // alloc object on heap, not stack!
}

