
#include "include.h"



//////////////////////////////


class CEarDefs
{
  protected:
          static const unsigned FDETECT_AUDIO_BUFFER_MSEC = 500;
          static const int GSM_MODEM_BAUDRATE = 57600; // on 115200 we can got some issues!
          static const unsigned MOBILE_TERMINAL_MAX_QUEUE_COMMANDS = 25;
          static const bool MOBILE_TERMINAL_AUTO_ANSWER_MODE = true;
          static const bool LOG_STDOUT_ECHO = true;
          static const unsigned DBG_AUDIO_FILE_LEN_IN_SEC = 1*60*60;  // 1 hour
          static const bool CLEAR_DBG_AUDIO_FILE = false;  // slowdown at startup if true!
          static const unsigned GSM_STATUS_UPDATE_INTERVAL = 5000;
          static const unsigned PING_INTERVAL = 3*60*60*1000;  // 3 hours

};


//////////////////////////////


// interface class for sending packets
class CPacketSender
{
  protected:
          static const unsigned MAX_QUEUED_PACKETS = 10;   // only for important packets

          CLog *p_log;
  public:
          CPacketSender(CLog *_log) : p_log(_log) {}
          virtual ~CPacketSender() {}

          virtual void PushImportant(const std::string& pkt) = 0;
          virtual void PushUnimportant(const std::string& pkt) = 0;
          virtual bool IsBusy() const = 0;
          virtual void Poll() = 0;

  protected:
          void SaveOffline(const std::string& pkt);

};


// class for sending packets offline only (used for debug)
class CPacketSenderOffline : public CPacketSender
{
  public:
          CPacketSenderOffline(CLog *_log) : CPacketSender(_log) {}

          void PushImportant(const std::string& pkt) { SaveOffline(pkt); }
          void PushUnimportant(const std::string& pkt) { /*SaveOffline(pkt);*/ }
          bool IsBusy() const { return false; }
          void Poll() {}
};


// class for sending packets via internet, sms, or file (offline)
class CPacketSenderMobile : public CPacketSender
{
          const CConfig& m_cfg;
          CTelitMobile *p_mob;

          enum {
           STAGE_INET,
           STAGE_SMS,
          };

          typedef struct {
           std::string data;
           int stage;
           bool is_important;
          } PACKET;

          PACKET *p_curr;   // this packet is pushed to terminal and we are waiting for answer

          typedef std::vector<PACKET*> TQueue;
          TQueue m_queue;

  public:
          CPacketSenderMobile(const CConfig& cfg,CTelitMobile *mob,CLog *_log);
          ~CPacketSenderMobile();

          void PushImportant(const std::string& pkt) { Push(pkt,true); }
          void PushUnimportant(const std::string& pkt) { Push(pkt,false); }
          bool IsBusy() const { return p_curr || !m_queue.empty(); }
          void Poll();

  private:
          void Push(const std::string& pkt,bool is_important);
          bool SendByInternet(const std::string& pkt);
          bool SendBySMS(const std::string& pkt);
          static void CallbackWrapper(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok);
          void Callback(int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok);
};


// class for sending packets via LoRa, RS485, or file (offline)
class CPacketSenderNew : public CPacketSender
{
          // these all options must be the same on the receiver's side!
          // (other LoRa options are set by default in TRadio constructor)
          static const CLoraMote::ERadioFreq    LORA_FREQ  = CLoraMote::RF_868MHz;
          static const CLoraMote::EBandWidth    LORA_BW    = CLoraMote::BW_500KHz;
          static const CLoraMote::ESpreadFactor LORA_SF    = CLoraMote::SF_7;
          static const CLoraMote::ECodingRate   LORA_CR    = CLoraMote::CR_4_5;
          static const int                      RS485_RATE = 9600;

          static const unsigned MAX_RANDOM_DELAY = 2000;
          static const unsigned MAX_ATTEMPTS = 3;   // for important packets only
          
          const CConfig& m_cfg;
          CUART *p_uart;
          CLoraMote *p_lora;

          typedef struct {
           std::string data;
           unsigned counter;  // from N attempts down to 0
           unsigned last_sent_time;
           unsigned random_delay;
          } PACKET;

          typedef std::vector<PACKET*> TQueue;
          TQueue m_queue;

  public:
          CPacketSenderNew(const CConfig& cfg,CLog *_log);
          ~CPacketSenderNew();

          void PushImportant(const std::string& pkt) { Push(pkt,true); }
          void PushUnimportant(const std::string& pkt) { Push(pkt,false); }
          bool IsBusy() const { return !m_queue.empty(); }
          void Poll();

  private:
          void Push(const std::string& pkt,bool is_important);
          unsigned GenRandomDelay();
          void SendPacket_Internal(const std::string& data);
};



void CPacketSender::SaveOffline(const std::string& pkt)
{
  FIL f;
  if ( f_open(&f,OFFLINE_FILENAME,FA_WRITE|FA_OPEN_APPEND) == FR_OK )
     {
       UINT wb = 0;
       f_write(&f,pkt.data(),pkt.size(),&wb);
       f_write(&f,"\r\n",2,&wb);
       f_close(&f);
     }
}


CPacketSenderMobile::CPacketSenderMobile(const CConfig& cfg,CTelitMobile *mob,CLog *_log)
  : CPacketSender(_log), m_cfg(cfg), p_mob(mob)
{
  p_curr = NULL;

  m_queue.reserve(MAX_QUEUED_PACKETS+1);  // +1 needed in case SMS-sending
}


CPacketSenderMobile::~CPacketSenderMobile()
{
  SAFEDELETE(p_curr);

  for ( unsigned n = 0; n < m_queue.size(); n++ )
      {
        SAFEDELETE(m_queue[n]);
      }
  m_queue.clear();
}


void CPacketSenderMobile::Push(const std::string& pkt,bool is_important)
{
  if ( !pkt.empty() )
     {
       if ( !IsBusy() || is_important )
          {
            if ( m_queue.size() < MAX_QUEUED_PACKETS )
               {
                 PACKET *p = new PACKET;

                 p->data = pkt;
                 p->stage = STAGE_INET;
                 p->is_important = is_important;
                 
                 m_queue.push_back(p);
               }
            else
               {
                 ADD2LOG(("PacketSender queue is full, save packet offline"));
                 SaveOffline(pkt);
               }
          }
     }
}


// returns false in case terminal queue is full
bool CPacketSenderMobile::SendByInternet(const std::string& pkt)
{
  int id = p_mob->InitiateInternetConnectionAndSendStringUDP(m_cfg.modem_old_firmware,m_cfg.apn.c_str(),NULL,NULL,
                                          m_cfg.server.c_str(),m_cfg.port_udp,pkt.c_str(),CallbackWrapper,this);
  return id >= 0;
}


// returns false in case terminal queue is full
bool CPacketSenderMobile::SendBySMS(const std::string& pkt)
{
  int id = p_mob->SendSMS(m_cfg.sms_number.c_str(),(m_cfg.sms_prefix+pkt).c_str(),CallbackWrapper,this);
  return id >= 0;
}


void CPacketSenderMobile::CallbackWrapper(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  reinterpret_cast<CPacketSenderMobile*>(parm)->Callback(id,cmd,answer,is_timeout,is_answered_ok);
}


void CPacketSenderMobile::Callback(int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( p_curr )  // paranoja
     {
       if ( is_timeout || !is_answered_ok )  //error?
          {
            if ( p_curr->is_important )
               {
                 if ( p_curr->stage == STAGE_INET )
                    {
                      ADD2LOG(("Error sending packet via Internet: %s",NNS(answer)));
                      if ( !m_cfg.sms_number.empty() )  // allow SMS?
                         {
                           p_curr->stage = STAGE_SMS;
                           m_queue.insert(m_queue.begin(),p_curr);
                           p_curr = NULL;
                         }
                      else
                         {
                           SaveOffline(p_curr->data);
                         }
                    }
                 else
                    {
                      ADD2LOG(("Error sending packet via SMS: %s",NNS(answer)));
                      SaveOffline(p_curr->data);
                    }
               }
          }
     
       SAFEDELETE(p_curr);
     }
}


void CPacketSenderMobile::Poll()
{
  if ( !p_curr && !m_queue.empty() )
     {
       p_curr = m_queue[0];  // we must support syncronous callbacks from SendBy... also!
       m_queue.erase(m_queue.begin());

       bool is_ok = (p_curr->stage == STAGE_INET ? SendByInternet(p_curr->data) : SendBySMS(p_curr->data));

       if ( !is_ok )
          {
            // terminal queue is full, nothing is pushed, no callbacks executed
            // restore back p_curr pointer
            assert(p_curr);
            m_queue.insert(m_queue.begin(),p_curr);
            p_curr = NULL;
          }
     }
}


CPacketSenderNew::CPacketSenderNew(const CConfig& cfg,CLog *_log)
  : CPacketSender(_log), m_cfg(cfg)
{
  p_uart = NULL;
  p_lora = NULL;

  if ( cfg.use_lora )
     {
       CLoraMote::TRadio i;  // here is default constructor
       i.freq = LORA_FREQ;
       i.bw = LORA_BW;
       i.sf = LORA_SF;
       i.cr = LORA_CR;

       p_lora = new CBoardLoraMote(i);
     }

  if ( cfg.use_rs485 )
     {
       p_uart = new CBoardRS485(RS485_RATE,false/*rx*/,true/*tx*/);
     }

  m_queue.reserve(MAX_QUEUED_PACKETS);
}


CPacketSenderNew::~CPacketSenderNew()
{
  for ( unsigned n = 0; n < m_queue.size(); n++ )
      {
        SAFEDELETE(m_queue[n]);
      }
  m_queue.clear();

  SAFEDELETE(p_uart);
  SAFEDELETE(p_lora);
}


void CPacketSenderNew::Poll()
{
  TQueue::iterator it = m_queue.begin();
  while ( it != m_queue.end() )
  {
    PACKET *p = *it;

    if ( GetTickCount() - p->last_sent_time > p->random_delay )
       {
         SendPacket_Internal(p->data);

         p->counter--;
         if ( p->counter == 0 )
            {
              delete p;
              it = m_queue.erase(it);
              continue;
            }
         else
            {
              p->last_sent_time = GetTickCount();
              p->random_delay = GenRandomDelay();
            }
       }

    ++it;
  }
}


void CPacketSenderNew::Push(const std::string& pkt,bool is_important)
{
  if ( !pkt.empty() )
     {
       if ( !IsBusy() || is_important )
          {
            if ( m_queue.size() < MAX_QUEUED_PACKETS )
               {
                 PACKET *p = new PACKET;

                 p->data = pkt;
                 p->counter = (is_important ? MAX_ATTEMPTS : 1);
                 p->last_sent_time = GetTickCount();
                 p->random_delay = GenRandomDelay();
                 
                 m_queue.push_back(p);
               }
            else
               {
                 // todo: insert packet instead of one in queue which was sent

                 ADD2LOG(("PacketSender queue is full, save packet offline"));
                 SaveOffline(pkt);
               }
          }
     }
}


unsigned CPacketSenderNew::GenRandomDelay()
{
  return rand() % MAX_RANDOM_DELAY;
}


void CPacketSenderNew::SendPacket_Internal(const std::string& data)
{
  if ( p_uart )
     {
       for ( unsigned n = 0; n < data.size(); n++ )
           {
             p_uart->SendByte((unsigned char)data[n]);
           }

       p_uart->SendByte('\r');
       p_uart->SendByte('\n');
     }

  if ( p_lora )
     {
       if ( !p_lora->Send(data.data(),data.size(),3000) )
          {
            // should be never happens!
            ADD2LOG(("Error: LoRa sending failed due to timeout"));
          }
     }
}


///////////////////////////


class CBalanceReq
{
          static const unsigned REQ_PERIOD = 1*60*60*1000;  // 1 hour, USSD request period
          
          const CConfig& m_cfg;
          CTelitMobile *p_mob;
          CLog *p_log;

          std::string m_answer;
          bool is_busy;

          unsigned last_req_time;

  public:
          CBalanceReq(const CConfig& cfg,CTelitMobile *mob,CLog *_log);
          ~CBalanceReq();

          std::string GetResult() const { return m_answer; }
          bool IsBusy() const { return is_busy; }
          void Poll();

  private:
          static void CallbackWrapper(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok);
          void Callback(int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok);

};


CBalanceReq::CBalanceReq(const CConfig& cfg,CTelitMobile *mob,CLog *_log)
  : m_cfg(cfg), p_mob(mob), p_log(_log)
{
  m_answer = "";
  is_busy = false;
  last_req_time = GetTickCount()-REQ_PERIOD+60000;
}


CBalanceReq::~CBalanceReq()
{
}


void CBalanceReq::Poll()
{
  if ( !is_busy )
     {
       if ( GetTickCount() - last_req_time > REQ_PERIOD )
          {
            if ( !m_cfg.ussd_balance.empty() )
               {
                 if ( p_mob->InitUSSDRequest(m_cfg.ussd_balance.c_str(),CallbackWrapper,this) >= 0 )
                    {
                      is_busy = true;
                      last_req_time = GetTickCount();
                    }
               }
            else
               {
                 last_req_time = GetTickCount();
               }
          }
     }
}


void CBalanceReq::CallbackWrapper(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  reinterpret_cast<CBalanceReq*>(parm)->Callback(id,cmd,answer,is_timeout,is_answered_ok);
}


void CBalanceReq::Callback(int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( is_busy )  // paranoja
     {
       if ( !is_timeout && is_answered_ok )
          {
            m_answer = CTelitMobile::DecodeUSSDAnswer(answer);
          }
       else
          {
            // for debug only:
            //ADD2LOG(("USSD Callback error (timeout=%s): %s",is_timeout?"YES":"no",answer));
          }
       
       is_busy = false;
     }
}


///////////////////////////


class CEar : public CEarDefs
{
          static const short INVALID_GNSS = 0x3f3f;  // '??'
          
          
          CLED* m_leds[4];   // base objects
          CLED* p_led1;      // pointer alias
          CLED* p_led2;      // pointer alias
          CLED* p_led3;      // pointer alias
          CLED* p_led4;      // pointer alias
          CLED* p_led_mic;   // pointer alias
          CLED* p_led_gsm;   // pointer alias
          CLED* p_led_sync;  // pointer alias
          CLED* p_led_nosim; // pointer alias

          FATFS m_ffs;

          CConfig m_cfg;

          CDebugAudio *p_debug_audio;

          CSatellite *p_sat;
          CTelitMobile *p_mob;

          CFDetector *p_fdetect;

          CLog *p_log;

          CPacketSender *p_sender;
          CBalanceReq *p_balance;

          CButton *p_btn1;
          CButton *p_btn2;
          CButton *p_btn3;

          bool b_synced;  // is we are time syncronized with satellite? if true CRTC::GetTime() returns true UTC time
          double m_lat;   // initially set to 0
          double m_lon;   // initially set to 0
          short m_gnss;   // initially set to INVALID_GNSS

          bool is_btn1_pressed;
          bool is_btn2_pressed;
          bool is_btn3_pressed;

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
          std::string PrepareFDetectPacket(OURTIME event_time,unsigned len_ms,float db_amp);
          void UpdateSync(unsigned& last_update_time);
          void UpdateLatLon(unsigned& last_update_time);
          void UpdateGSMStatuses(unsigned& last_update_time);
          void UpdatePing(unsigned& last_update_time);
          void UpdateBalance(unsigned& last_update_time);
          void CheckButtonsPressed();
          void OnButton1();
          void OnButton2();
          void OnButton3();
          void CheckFDetect();
          void PollObjects();
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
  p_debug_audio = NULL;
  p_sat = NULL;
  p_mob = NULL;
  p_fdetect = NULL;
  p_log = NULL;
  p_sender = NULL;
  p_balance = NULL;
  p_btn1 = NULL;
  p_btn2 = NULL;
  p_btn3 = NULL;
  b_synced = false;
  m_lat = 0.0;
  m_lon = 0.0;
  m_gnss = INVALID_GNSS;
  is_btn1_pressed = false;
  is_btn2_pressed = false;
  is_btn3_pressed = false;


  // system init
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);  // 4 bits for preemption priority, 0 for subpriority

  CRandom::Init();
  srand(CRandom::Get());  // use system rng only for pseudo-random seed
  CRandom::Done();
  
  CCPUTicks::Init();
  CSysTicks::Init();
  CDebugger::Init();
  // start from this point we can use printf()!

  Sleep(1000);

  m_leds[0] = new CLED(BOARD_LED1);
  m_leds[1] = new CLED(BOARD_LED2);
  m_leds[2] = new CLED(BOARD_LED3);
  m_leds[3] = new CLED(BOARD_LED4);
  p_led1 = m_leds[0];
  p_led2 = m_leds[1];
  p_led3 = m_leds[2];
  p_led4 = m_leds[3];
  p_led_mic = p_led1;
  p_led_gsm = p_led2;
  p_led_sync = p_led3;
  p_led_nosim = p_led4;
  // start from this point we can use FatalError()

  Beep(1000,100);  //Sleep(100);  // paranoja

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

  if ( m_cfg.use_gsm && (m_cfg.use_lora || m_cfg.use_rs485) )
     {
       printf("Config mismatch: use_gsm && (use_lora || use_rs485)\n");
       FatalError();
     }

  p_led3->On();
  p_debug_audio = m_cfg.debug_mode ? new CDebugAudio(CMic::SAMPLE_RATE,DBG_AUDIO_FILENAME,DBG_AUDIO_FILE_LEN_IN_SEC,CLEAR_DBG_AUDIO_FILE) : NULL;
  p_led3->Off();

  p_sat = new CSatellite(m_cfg.gps_baud,m_cfg.use_gps,m_cfg.use_glonass,m_cfg.use_galileo,m_cfg.use_beidou);

  if ( !CRTC::Init(5,5,5,RTC_Weekday_Thursday,0,0,0,IRQ_OnTimeStamp,p_sat) )
     {
       printf("RTC init failed\n");
       FatalError();
     }
  // start from this point we can use CRTC::XXX

  p_log = new CLog(LOG_FILENAME,LOG_STDOUT_ECHO);
  // start from this point we can use ADD2LOG()

  if ( m_cfg.use_gsm )
     {
       p_mob = new CTelitMobile(GSM_MODEM_BAUDRATE,MOBILE_TERMINAL_MAX_QUEUE_COMMANDS);
       if ( !p_mob->Startup(MOBILE_TERMINAL_AUTO_ANSWER_MODE) )
          {
            ADD2LOG(("Mobile init failed"));
            //FatalError();  - not critical
          }
     }

  p_fdetect = new CFDetector(CMic::SAMPLE_RATE,FDETECT_AUDIO_BUFFER_MSEC);

  CMic::Init(IRQ_OnMicWrapper,this);  // after creating p_fdetect!

  if ( m_cfg.use_gsm )
     {
       p_sender = new CPacketSenderMobile(m_cfg,p_mob,p_log);
       p_balance = new CBalanceReq(m_cfg,p_mob,p_log);
     }
  else
  if ( m_cfg.use_lora || m_cfg.use_rs485 )
     {
       p_sender = new CPacketSenderNew(m_cfg,p_log);
     }
  else
     {
       p_sender = new CPacketSenderOffline(p_log);
     }

  p_btn1 = new CButton(BOARD_BUTTON1,IRQ_OnButton1Wrapper,this);
  p_btn2 = new CButton(BOARD_BUTTON2,IRQ_OnButton2Wrapper,this);
  p_btn3 = new CButton(BOARD_BUTTON3,IRQ_OnButton3Wrapper,this);
}


CEar::~CEar()
{
  assert(false);
}


void CEar::FatalError()
{
  __disable_irq();
  
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
  is_btn1_pressed = true;
}


// WARNING!!! This is an IRQ handler!
void CEar::IRQ_OnButton2()
{
  is_btn2_pressed = true;
}


// WARNING!!! This is an IRQ handler!
void CEar::IRQ_OnButton3()
{
  is_btn3_pressed = true;
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
  // LED
  int sum = 0;
  for ( int n = 0; n < num_samples; n++ )
      {
        int s = pcm_buff[n];
        if ( s < 0 )
         s = -s;
        sum += s;
      }
  sum /= num_samples;
  p_led_mic->SetState(sum>2000);

  // Fight detect
  assert(num_samples==CMic::SAMPLE_RATE/1000);
  p_fdetect->Push1ms(GetTickCount(),pcm_buff);

  // debug audio
  if ( p_debug_audio )
     {
       p_debug_audio->Push1ms(pcm_buff);
     }
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
  i.header.fdetect_ver = CFDetector::VERSION;
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
  i->header.fdetect_ver = CFDetector::VERSION;
  strcpy(i->text,text);

  return EncodePacket(i,size);
}


std::string CEar::PrepareFDetectPacket(OURTIME event_time,unsigned len_ms,float db_amp)
{
  TCmdFDetect i;

  i.header.cmd_id = CMDID_FDETECT;
  i.header.sector = m_cfg.sector;
  i.header.device_id = m_cfg.device_id;
  i.header.client_ver = CLIENT_VER;
  i.header.fdetect_ver = CFDetector::VERSION;
  i.time_utc = event_time;
  i.geo.lat = GEO2INT(m_lat);
  i.geo.lon = GEO2INT(m_lon);
  i.fight_len_ms = len_ms;
  i.fight_db_amp = db_amp;

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

            if ( sync )
               {
                 double lat,lon;
                 short gnss;
                 if ( p_sat->GetNavData(lat,lon,gnss) )
                    {
                      ADD2LOG(("Coords: %.7f, %.7f",lat,lon));
                    }
               }
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


void CEar::UpdateGSMStatuses(unsigned& last_update_time)
{
  if ( p_mob && p_balance )
     {
       if ( GetTickCount() - last_update_time > GSM_STATUS_UPDATE_INTERVAL )
          {
            if ( !p_sender->IsBusy() && !p_balance->IsBusy() )  // to not fill terminal's queue if busy
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
       p_led_gsm->SetState(p_mob->GetNetStatus()==NET_REGISTERED_HOME||p_mob->GetNetStatus()==NET_REGISTERED_ROAMING);
     }
}


void CEar::UpdatePing(unsigned& last_update_time)
{
  if ( GetTickCount() - last_update_time > PING_INTERVAL )
     {
       ADD2LOG(("Ping..."));

       p_sender->PushUnimportant(PreparePingPacket());

       last_update_time = GetTickCount();
     }
}


void CEar::UpdateBalance(unsigned& last_update_time)
{
  if ( p_balance )
     {
       if ( GetTickCount() - last_update_time > PING_INTERVAL )
          {
            ADD2LOG(("Balance..."));

            std::string text = p_balance->GetResult();  // can return empty string
            p_sender->PushUnimportant(PrepareUSSDPacket(text.c_str()));

            last_update_time = GetTickCount();
          }
     }
}


void CEar::CheckButtonsPressed()
{
  if ( is_btn1_pressed || is_btn2_pressed || is_btn3_pressed )
     {
       Sleep(400);

       if ( is_btn1_pressed )
          {
            OnButton1();
          }
       else
       if ( is_btn2_pressed )
          {
            OnButton2();
          }
       else
       if ( is_btn3_pressed )
          {
            OnButton3();
          }

       is_btn1_pressed = false;
       is_btn2_pressed = false;
       is_btn3_pressed = false;
     }
}


// "stop processing" button
void CEar::OnButton1()
{
  if ( p_debug_audio )
     {
       p_debug_audio->Stop();
     }

  ADD2LOG(("Stopped by user!"));
  ADD2LOG(("----------------------"));

  FatalError();
}


// "ping" button
void CEar::OnButton2()
{
  p_sender->PushUnimportant(PreparePingPacket());
}


// "balance" button
void CEar::OnButton3()
{
  if ( p_balance )
     {
       std::string text = p_balance->GetResult();  // can return empty string
       
       ADD2LOG(("Balance: %s",text.c_str()));

       p_sender->PushUnimportant(PrepareUSSDPacket(text.c_str()));
     }
}


void CEar::CheckFDetect()
{
  unsigned ts,length_ms;
  float db_amp;
  bool is_detected = p_fdetect->PopResult(ts,length_ms,db_amp);
  if ( is_detected )
     {
       p_led1->On();
       p_led2->On();
       p_led3->On();
       p_led4->On();
       Sleep(100);
       p_led1->Off();
       p_led2->Off();
       p_led3->Off();
       p_led4->Off();

       if ( !IsSynced() )
          {
            ADD2LOG(("FDetect WITHOUT time sync: %d msec, %.1f dB",length_ms,db_amp));
          }
       else
          {
            OURTIME ftime = CRTC::GetTime() - (OURTIME)(GetTickCount()-ts);

            ADD2LOG(("FDetect: %s, %d msec, %.1f dB, [%.7f, %.7f]",COurTime(ftime).AsString().c_str(),length_ms,db_amp,m_lat,m_lon));

            p_sender->PushImportant(PrepareFDetectPacket(ftime,length_ms,db_amp));  // not always 100% async!
          }
     }
}


void CEar::PollObjects()
{
  p_sender->Poll();  // for old firmware GSM modem can hung for many seconds!!!

  if ( p_balance )
     {
       p_balance->Poll();
     }

  p_sat->Poll();

  if ( p_mob )
     {
       p_mob->Poll();
     }

  if ( p_debug_audio )
     {
       p_debug_audio->Poll();
     }
}


void CEar::MainLoop()
{
  ADD2LOG(("--- System started ---"));
  ADD2LOG(("device_id: %d, sector: %d",m_cfg.device_id,m_cfg.sector));
  ADD2LOG(("GPS:%d, Glonass:%d, Galileo:%d, Beidou:%d",m_cfg.use_gps,m_cfg.use_glonass,m_cfg.use_galileo,m_cfg.use_beidou));
  ADD2LOG(("Debug mode: %s",m_cfg.debug_mode?"YES":"no"));
  ADD2LOG(("GSM:%d, LoRa:%d, RS485:%d",m_cfg.use_gsm,m_cfg.use_lora,m_cfg.use_rs485));

  unsigned last_update_sync = GetTickCount();
  unsigned last_update_latlon = GetTickCount();
  unsigned last_update_gsm = GetTickCount()-GSM_STATUS_UPDATE_INTERVAL;
  unsigned last_update_ping = GetTickCount()-PING_INTERVAL+120000;
  unsigned last_update_balance = last_update_ping+30000;

  while ( 1 )
  {
    UpdateSync(last_update_sync);
    UpdateLatLon(last_update_latlon);
    UpdateGSMStatuses(last_update_gsm); 
    UpdatePing(last_update_ping);
    UpdateBalance(last_update_balance);
    CheckFDetect();
    CheckButtonsPressed();
    PollObjects();
  }
}



//////////////////


int main()
{
  (new CEar())->MainLoop();   // alloc object on heap, not stack!
}

