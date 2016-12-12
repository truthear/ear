
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
class CTelitMobile
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
          // constructor takes about ~300 msec!
          CTelitMobile(int rate,unsigned max_queue_cmds); // each queued cmd allocates about ~64+cmd_len bytes in heap!
          ~CTelitMobile();

          // to work properly these function needs a calls to correspondent UpdateXXX() functions
          ESIMStatus GetSIMStatus() const { return m_sim_status; }
          ENetStatus GetNetStatus() const { return m_net_status; }
          ENetStatus GetGPRSStatus() const { return m_gprs_status; }
          int GetSignalQuality() const { return m_signal_quality; }
          bool GetInternetConnectionStatus() const { return b_internet_connected; }

          void Poll();

          void ResetModem();  // can take a time! it is recommended to execute Startup() again after ResetModem()!

          // list of SYNC commands:
          bool Startup(bool use_auto_answer_mode=true);  // execute base init AT-cmds

          // list of ASYNC commands, they returns -1 if queue is full, or unique id for use in callbacks otherwise
          void UpdateSIMStatus();
          void UpdateNetStatus();
          void UpdateGPRSStatus();
          void UpdateSignalQuality();
          void UpdateInternetConnectionStatus();
          int InitUSSDRequest(const char *ussd,CTerminal::TCALLBACK cb,void *cbparm=NULL,unsigned max_time_to_wait_answer=10000);
          static std::string DecodeUSSDAnswer(const char *answer);
          // Warning! Text conversion is not performed here!
          int SendSMS(const char *phone,const char *text,CTerminal::TCALLBACK cb,void *cbparm=NULL,unsigned timeout=60000);
          int InitiateInternetConnection(const char *apn,const char *user=NULL,const char *pwd=NULL,unsigned timeout=60000);
          int ShutdownInternetConnection(unsigned timeout=15000);
          // Warning! Text conversion is not performed here!
          // Warning! InitiateInternetConnection() should be called first
          // WARNING!!! symbols \r,\n is not permitted inside str!
          int SendStringTCP(const char *server,int port,const char *str,CTerminal::TCALLBACK cb,void *cbparm=NULL,
                            unsigned conn_timeout=10000,unsigned total_timeout=35000);
          int SendStringUDP(const char *server,int port,const char *str,CTerminal::TCALLBACK cb,void *cbparm=NULL,
                            unsigned total_timeout=25000);

          // use it for old firmware only!
          // function can take a little time, not 100% async!
          // WARNING!!! symbols \r,\n is not permitted inside str!
          int SendStringUDP_OldFW(const char *server,int port,const char *str,CTerminal::TCALLBACK cb,void *cbparm=NULL,
                                  unsigned total_timeout=25000);

          // high level function with default timeouts:
          int InitiateInternetConnectionAndSendStringUDP(bool use_old_fw,const char *apn,const char *user,const char *pwd,
                                                         const char *server,int port,const char *str,
                                                         CTerminal::TCALLBACK cb,void *cbparm);

  private:
          static void GeneralStatusCBWrapper(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok);
          void GeneralStatusCB(int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok);
          static ENetStatus GetNetRegResultInternal(bool is_answered_ok,const char *answer,const char *srch);

};



#endif
