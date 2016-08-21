
#ifndef __SERVER_H__
#define __SERVER_H__


class CServer
{
          static const unsigned MAX_TCP_CONNECTIONS = 300;
          static const unsigned MAX_INCOMING_PACKET_SIZE = 1024;  // allocated in stack, should be not more than 64K for UDP!

          TCFG m_cfg;

          HANDLE h_stop_event;
          HANDLE h_thread;

  public:
          CServer();
          ~CServer();

  private:
          static DWORD WINAPI ThreadProcWrapper(void*);
          void ThreadProc();
          void BindSocket(SOCKET s,unsigned short port,const char *proto_desc);
          std::string DecodePacket(const std::string& b64);
          void DispatchPacket(const std::string& bin,BOOL is_from_localhost);
          void OnServerPing(const TCmdServerPing& cmd,BOOL is_from_sms);
          void OnUSSDBalance(const TCmdUSSDBalance& cmd,BOOL is_from_sms);
          void OnFDetect(const TCmdFDetect& cmd,BOOL is_from_sms);
          void SaveFDetectInfo2DB(const TCmdFDetect& cmd,BOOL is_from_sms);
          void SaveCalculationResult2DB(int sector,OURTIME time_utc,double lat,double lon,int numsources);
};



#endif
