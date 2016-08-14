
#ifndef __MOBILE_H__
#define __MOBILE_H__



enum ESIMStatus {
 SIM_ERROR    = 0,
 SIM_READY    = 1,
 SIM_NOTREADY = 2,
};

enum ENetStatus {
 NET_NOT_REGISTERED     = 0,
 NET_REGISTERED_HOME    = 1,
 NET_SEARCHING          = 2,
 NET_DENIED             = 3,
 NET_UNKNOWN            = 4,
 NET_REGISTERED_ROAMING = 5,
};


class CBoardModem;
class CTerminal;


// Telit-specific mobile tools class
// this class uses CSysTicks, which should be initialized before!
// WARNING!!! class not IRQ safe!
class CMobile
{
          static const unsigned RECV_BUFF_SIZE = 1024;  // should include AT-cmd length for echo output

          int m_baudrate;
          CBoardModem *p_modem;

          CTerminal *p_trm;

          ESIMStatus m_sim_status;
          ENetStatus m_net_status;
          ENetStatus m_gprs_status;
          int m_signal_quality;
          bool b_internet_connected;

  public:
          CMobile(int rate,unsigned max_queue_cmds=20);
          ~CMobile();

          void Poll();
          void ResetModem();  // can take a time!

          // list of SYNC commands:
          bool Startup(bool use_auto_answer_mode=true);  // execute base init AT-cmds

          // list of ASYNC commands:
          void UpdateSIMStatus();
          ESIMStatus GetSIMStatus() const { return m_sim_status; }
          void UpdateNetStatus();
          ENetStatus GetNetStatus() const { return m_net_status; }
          void UpdateGPRSStatus();
          ENetStatus GetGPRSStatus() const { return m_gprs_status; }
          void UpdateSignalQuality();
          int GetSignalQuality() const { return m_signal_quality; }
          void UpdateInternetConnectionStatus();
          bool GetInternetConnectionStatus() const { return b_internet_connected; }
          int InitUSSDRequest(const char *ussd,CTerminal::TCALLBACK cb,void *cbparm=NULL,unsigned max_time_to_wait_answer=10000);
          static std::string DecodeUSSDAnswer(const char *answer);
          // Warning! Text conversion is not performed here!
          int SendSMS(const char *phone,const char *text,CTerminal::TCALLBACK cb,void *cbparm=NULL,unsigned timeout=15000);
          void InitiateInternetConnection(const char *apn,const char *user=NULL,const char *pwd=NULL,unsigned timeout=15000);
          void ShutdownInternetConnection(unsigned timeout=3000);
          // Warning! Text conversion is not performed here!
          // Warning! InitiateInternetConnection() should be called first
          int SendStringTCP(const char *server,int port,const char *str,CTerminal::TCALLBACK cb,void *cbparm=NULL,unsigned conn_timeout=10000,unsigned total_timeout=15000);

  private:
          static void GeneralStatusCBWrapper(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok);
          void GeneralStatusCB(int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok);
};



#endif
