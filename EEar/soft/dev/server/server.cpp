
#include "include.h"



CServer::CServer()
{
  #ifdef _WIN64
  const char *s_system = "x64";
  #else
  const char *s_system = "x86";
  #endif
  ADD2LOG(("--- Server %s (%s) started (built %s) ---",GLOBAL_VERSION_S,s_system,__DATE__ " " __TIME__));

  ReadConfig(m_cfg);

  p_analyzer = NULL;

  h_stop_event = CreateEvent(NULL,FALSE,FALSE,NULL);
  h_thread = CreateThread(NULL,0,ThreadProcWrapper,this,0,NULL);
}


CServer::~CServer()
{
  SetEvent(h_stop_event);
  
  if ( WaitForSingleObject(h_thread,5000) == WAIT_TIMEOUT )
     {
       TerminateThread(h_thread,0);
       ADD2LOG(("Main thread abnormally terminated"));
     }
  CloseHandle(h_thread);
  h_thread = NULL;
  CloseHandle(h_stop_event);
  h_stop_event = NULL;

  ADD2LOG(("----------------------------"));
}


DWORD WINAPI CServer::ThreadProcWrapper(void *parm)
{
  reinterpret_cast<CServer*>(parm)->ThreadProc();
  return 1;
}


class CTCPClient
{
           SOCKET sckt;
           std::string recv_string;
           unsigned starttime;
   public:
           CTCPClient(SOCKET s)
           {
             sckt = s;
             starttime = GetTickCount();
           }
           ~CTCPClient()
           {
             FinishSocket(sckt);
           }
           BOOL IsTimeout() const
           {
             return GetTickCount() - starttime > 30000;
           }
           operator const SOCKET& () const
           {
             return sckt;
           }
           operator std::string& () 
           {
             return recv_string;
           }
};



void CServer::ThreadProc()
{
  SOCKET h_socket_udp = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
  BindSocket(h_socket_udp,m_cfg.port_udp,"UDP");

  SOCKET h_socket_tcp = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  BindSocket(h_socket_tcp,m_cfg.port_tcp,"TCP");
  listen(h_socket_tcp,SOMAXCONN);

  typedef std::vector<CTCPClient*> TTCPClients;
  TTCPClients clients;  // TCP only

  p_analyzer = new CHighLevelSectorsAnalyzer;
  
  // main loop
  do {

    // accept TCP clients
    if ( CanReadNonBlocked(h_socket_tcp) )
       {
         struct sockaddr_in s_in;
         int i_s_in_len = sizeof(s_in);
         SOCKET cl_sock = accept(h_socket_tcp,(struct sockaddr*)&s_in,&i_s_in_len);
         if ( cl_sock != INVALID_SOCKET )
            {
              if ( clients.size() < MAX_TCP_CONNECTIONS )
                 {
                   clients.push_back(new CTCPClient(cl_sock));
                 }
              else
                 {
                   FinishSocket(cl_sock);
                 }
            }
       }

    // read UDP datagrams
    if ( CanReadNonBlocked(h_socket_udp) )
       {
         char buff[MAX_INCOMING_PACKET_SIZE];
         struct sockaddr_in s_in;
         int i_s_in_len = sizeof(s_in);
         int rbytes = recvfrom(h_socket_udp,buff,sizeof(buff),0,(struct sockaddr*)&s_in,&i_s_in_len);
         if ( rbytes > 0 )
            {
              std::string b64(buff,(size_t)rbytes);
              std::string bin_packet = DecodePacket(b64);
              DispatchPacket(bin_packet,s_in.sin_addr.s_addr==0x0100007f);
            }
       }

    // poll TCP clients
    {
      BOOL b_deleted;
      do {

       b_deleted = FALSE;
       for ( TTCPClients::iterator it = clients.begin(); it != clients.end(); ++it )
           {
             CTCPClient *pcl = *it;
             CTCPClient& cl = *pcl;

             BOOL need_delete = FALSE;

             if ( CanReadNonBlocked(cl) )
                {
                  char buff[MAX_INCOMING_PACKET_SIZE];
                  int rbytes = recv(cl,buff,sizeof(buff),0);
                  if ( rbytes > 0 )
                     {
                       std::string& s = cl;
                       s += std::string(buff,(size_t)rbytes);
                       if ( s.size() > MAX_INCOMING_PACKET_SIZE )
                          {
                            need_delete = TRUE;
                          }
                     }
                  else
                     {
                       need_delete = TRUE;
                     }
                }
             else
                {
                  need_delete = cl.IsTimeout();
                }

             if ( need_delete )
                {
                  DispatchPacket(DecodePacket(cl),FALSE);  // try to dispatch data
                  SAFEDELETE(pcl);
                  clients.erase(it);
                  b_deleted = TRUE;
                  break;
                }
           }

      } while ( b_deleted );
    }

    // poll other
    PollResult();

    // wait
    if ( WaitForSingleObject(h_stop_event,50) == WAIT_OBJECT_0 )
       break;

  } while (1);

  SAFEDELETE(p_analyzer);

  // cleanup rest TCP clients
  for ( unsigned n = 0; n < clients.size(); n++ )
      {
        SAFEDELETE(clients[n]);
      }
  clients.clear();

  // close base sockets
  FinishSocket(h_socket_tcp);
  FinishSocket(h_socket_udp);
}


void CServer::BindSocket(SOCKET s,unsigned short port,const char *proto_desc)
{
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);
  
  if ( bind(s,(struct sockaddr*)&sin,sizeof(sin)) )
     {
       ADD2LOG(("bind() failed, %s:%d, error:%d",proto_desc,port,WSAGetLastError()));
     }
  else
     {
       ADD2LOG(("Bind to *:%d (%s)",port,proto_desc));
     }
}


std::string CServer::DecodePacket(const std::string& b64)
{
  std::string rc;

  if ( !b64.empty() )
     {
       std::string t = Base64Decode(b64.c_str());
       if ( !t.empty() )
          {
            if ( t.size() > sizeof(unsigned) && ((t.size()-sizeof(unsigned))%16) == 0 )
               {
                 const char *p = t.data();
                 unsigned len = t.size();

                 unsigned crc32 = *(const unsigned*)p; 
                 p += sizeof(unsigned); 
                 len -= sizeof(unsigned);

                 AESCONTEXT ai;
                 aes_setkey_dec(&ai,(const unsigned char*)OUR_AES_KEY,OUR_AES_KEY_SIZE);

                 unsigned numblocks = len/16;
                 for ( unsigned n = 0; n < numblocks; n++ )
                     {
                       unsigned char output[16];
                       
                       aes_crypt_ecb_dec(&ai,(const unsigned char*)p,output);
                       rc += std::string((char*)output,sizeof(output));
                       
                       p += 16;
                       len -= 16;
                     }

                 if ( CRC32(0,(const unsigned char*)rc.data(),rc.size()) != crc32 )
                    {
                      rc.clear();
                    }
               }
          }
     }

  return rc;
}


void CServer::DispatchPacket(const std::string& bin,BOOL is_from_localhost)
{
  if ( !bin.empty() )
     {
       unsigned char cmd = (unsigned char)bin[0];

       if ( cmd == CMDID_SERVER_PING )
          {
            if ( bin.size() >= sizeof(TCmdServerPing) )
               {
                 OnServerPing(*(const TCmdServerPing*)bin.data(),is_from_localhost);
               }
          }
       else
       if ( cmd == CMDID_USSD_BALANCE )
          {
            if ( bin.size() >= sizeof(TCmdHeader)+1 )
               {
                 OnUSSDBalance(*(const TCmdUSSDBalance*)bin.data(),is_from_localhost);
               }
          }
       else
       if ( cmd == CMDID_FDETECT )
          {
            if ( bin.size() >= sizeof(TCmdFDetect) )
               {
                 OnFDetect(*(const TCmdFDetect*)bin.data(),is_from_localhost);
               }
          }
       //else
       //  ...
     }
}


void CServer::OnServerPing(const TCmdServerPing& cmd,BOOL is_from_sms)
{
  OURTIME srv_time_lcl = GetNowOurTime();
  std::wstring srv_time_s = (const WCHAR*)CUnicode(OurTimeToString(srv_time_lcl).c_str());
  std::wstring cl_time_s = (const WCHAR*)CUnicode((OurTimeToString(cmd.time_utc)+" UTC").c_str());
  double lat = INT2GEO(cmd.geo.lat);
  double lon = INT2GEO(cmd.geo.lon);

  CLocalDB sql;

  sql.Exec(L"CREATE TABLE IF NOT EXISTS TPing(dev_id INT NOT NULL PRIMARY KEY,sector INT,cl_ver INT,fd_ver INT,last_ts_ms INT,cl_time_utc INT,cl_time_s TEXT,srv_time_lcl INT,srv_time_s TEXT,lat REAL,lon REAL)");

  CSQLiteQuery *q = sql.CreateQuery(L"UPDATE TPing SET sector=?,cl_ver=?,fd_ver=?,last_ts_ms=?,cl_time_utc=?,cl_time_s=?,srv_time_lcl=?,srv_time_s=?,lat=?,lon=? WHERE dev_id=?");
  q->BindAsInt(cmd.header.sector);
  q->BindAsInt(cmd.header.client_ver);
  q->BindAsInt(cmd.header.fdetect_ver);
  q->BindAsInt(cmd.last_timesync_ms);
  q->BindAsInt64(cmd.time_utc);
  q->BindAsText(cl_time_s);
  q->BindAsInt64(srv_time_lcl);
  q->BindAsText(srv_time_s);
  q->BindAsDouble(lat);
  q->BindAsDouble(lon);
  q->BindAsInt(cmd.header.device_id);
  BOOL need_insert = (q->Step() && sql.GetNumRowsAffected() == 0);
  q->Destroy();

  if ( need_insert )
     {
       CSQLiteQuery *q = sql.CreateQuery(L"INSERT INTO TPing VALUES(?,?,?,?,?,?,?,?,?,?,?)");
       q->BindAsInt(cmd.header.device_id);
       q->BindAsInt(cmd.header.sector);
       q->BindAsInt(cmd.header.client_ver);
       q->BindAsInt(cmd.header.fdetect_ver);
       q->BindAsInt(cmd.last_timesync_ms);
       q->BindAsInt64(cmd.time_utc);
       q->BindAsText(cl_time_s);
       q->BindAsInt64(srv_time_lcl);
       q->BindAsText(srv_time_s);
       q->BindAsDouble(lat);
       q->BindAsDouble(lon);
       q->Step();
       q->Destroy();
     }
}


void CServer::OnUSSDBalance(const TCmdUSSDBalance& cmd,BOOL is_from_sms)
{
  std::wstring srv_time_s = (const WCHAR*)CUnicode(OurTimeToString(GetNowOurTime()).c_str());

  char *txt = (char*)alloca(lstrlen(cmd.text)+1);  // not need to free
  lstrcpy(txt,cmd.text);
  StrTrim(txt," \r\n\t\"");
  std::wstring ussd = (const WCHAR*)CUnicode(txt);

  CLocalDB sql;

  sql.Exec(L"CREATE TABLE IF NOT EXISTS TBalance(dev_id INT NOT NULL PRIMARY KEY,srv_time_s TEXT,ussd TEXT)");

  CSQLiteQuery *q = sql.CreateQuery(L"UPDATE TBalance SET srv_time_s=?,ussd=? WHERE dev_id=?");
  q->BindAsText(srv_time_s);
  q->BindAsText(ussd);
  q->BindAsInt(cmd.header.device_id);
  BOOL need_insert = (q->Step() && sql.GetNumRowsAffected() == 0);
  q->Destroy();

  if ( need_insert )
     {
       CSQLiteQuery *q = sql.CreateQuery(L"INSERT INTO TBalance VALUES(?,?,?)");
       q->BindAsInt(cmd.header.device_id);
       q->BindAsText(srv_time_s);
       q->BindAsText(ussd);
       q->Step();
       q->Destroy();
     }
}


void CServer::OnFDetect(const TCmdFDetect& cmd,BOOL is_from_sms)
{
  SaveFDetectInfo2DB(cmd,is_from_sms);

  p_analyzer->PushSensorDetection(cmd.header.sector,cmd.header.device_id,cmd.header.fdetect_ver,
                                  cmd.fight_len_ms,cmd.fight_db_amp,
                                  INT2GEO(cmd.geo.lat),INT2GEO(cmd.geo.lon),cmd.time_utc);
}


void CServer::SaveFDetectInfo2DB(const TCmdFDetect& cmd,BOOL is_from_sms)
{
  CLocalDB sql;

  sql.Exec(L"CREATE TABLE IF NOT EXISTS TFDetect(dev_id INT,sector INT,cl_ver INT,fd_ver INT,event_time_utc INT,srv_time_lcl INT,lat REAL,lon REAL,len_ms INT,db_amp REAL,is_from_sms INT)");
  sql.Exec(L"CREATE INDEX IF NOT EXISTS IFDetect1 ON TFDetect(srv_time_lcl)");

  CSQLiteQuery *q = sql.CreateQuery(L"INSERT INTO TFDetect VALUES(?,?,?,?,?,?,?,?,?,?,?)");
  q->BindAsInt(cmd.header.device_id);
  q->BindAsInt(cmd.header.sector);
  q->BindAsInt(cmd.header.client_ver);
  q->BindAsInt(cmd.header.fdetect_ver);
  q->BindAsInt64(cmd.time_utc);
  q->BindAsInt64(GetNowOurTime());
  q->BindAsDouble(INT2GEO(cmd.geo.lat));
  q->BindAsDouble(INT2GEO(cmd.geo.lon));
  q->BindAsInt(cmd.fight_len_ms);
  q->BindAsDouble(cmd.fight_db_amp);
  q->BindAsInt(is_from_sms);
  q->Step();
  q->Destroy();
}


void CServer::SaveCalculationResult2DB(int sector,OURTIME time_utc,double lat,double lon,int numsources)
{
  CLocalDB sql;

  sql.Exec(L"CREATE TABLE IF NOT EXISTS TResult(sector INT,time_utc INT,lat REAL,lon REAL,numsources INT)");
  sql.Exec(L"CREATE INDEX IF NOT EXISTS IResult1 ON TResult(time_utc)");

  CSQLiteQuery *q = sql.CreateQuery(L"INSERT INTO TResult VALUES(?,?,?,?,?)");
  q->BindAsInt(sector);
  q->BindAsInt64(time_utc);
  q->BindAsDouble(lat);
  q->BindAsDouble(lon);
  q->BindAsInt(numsources);
  q->Step();
  q->Destroy();
}


void CServer::PollResult()
{
  int numsensors,sector,fdetect_ver;
  double lat,lon;
  OURTIME ts;
  
  while ( p_analyzer->PopResult(numsensors,sector,fdetect_ver,lat,lon,ts) )
  {
    SaveCalculationResult2DB(sector,ts,lat,lon,numsensors);
  }
}



