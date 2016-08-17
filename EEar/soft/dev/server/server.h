
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
          void DispatchPacket(const std::string& bin);
};



#endif
