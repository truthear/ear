
#include "include.h"




void ProcessLog(TMODINTF *i)
{
  CHTML out(i,L"Server Log");

  {
    CPre pre(i);

    {
      CReadDBTable db(L"SELECT ev_time,ev_desc FROM TLog ORDER BY ev_time DESC LIMIT 50000");

      while ( db.FetchRow() )
      {
        out += HTMLFilter("["+OurTimeToString(db.GetAsInt64(0))+"] ");
        out += HTMLFilter(db.GetAsText(1));
        out += "\n";
      }
    }
  }
}


void ProcessPing(TMODINTF *i)
{
  CHTML out(i,L"Clients Ping");

  {
    CReadDBTable db(L"SELECT dev_id,sector,cl_ver,fd_ver,last_ts_ms,cl_time_s,srv_time_s,lat,lon FROM TPing ORDER BY srv_time_lcl DESC");

    CTable tbl(i);

    {
      CTableRow r(i);
      { CTableCellHeader c(i); out += HTMLFilter("dev_id"); }
      { CTableCellHeader c(i); out += HTMLFilter("sector"); }
      { CTableCellHeader c(i); out += HTMLFilter("version"); }
      { CTableCellHeader c(i); out += HTMLFilter("last_ts (msec)"); }
      { CTableCellHeader c(i); out += HTMLFilter("client time (UTC)"); }
      { CTableCellHeader c(i); CUnderline u(i); out += HTMLFilter("SERVER TIME (LOCAL)"); }
      { CTableCellHeader c(i); out += HTMLFilter("geo location"); }
    }

    while ( db.FetchRow() )
    {
      CTableRow r(i);
      { CTableCell c(i); out += db.GetAsInt(0); }
      { CTableCell c(i); out += db.GetAsInt(1); }
      { CTableCell c(i); out += HTMLFilter(CFormat("v%X (%X)",db.GetAsInt(2),db.GetAsInt(3))); }
      { CTableCell c(i); out += db.GetAsInt(4); }
      { CTableCell c(i); out += HTMLFilter(db.GetAsText(5)); }
      { CTableCell c(i); out += HTMLFilter(db.GetAsText(6)); }
      { 
        CTableCell c(i); 
        std::string ll = CFormat("%.7f,%.7f",db.GetAsDouble(7),db.GetAsDouble(8));
        CAnchor a(i,CFormat("https://maps.google.com/maps?ll=%s&spn=0.001,0.001&t=m&q=%s",ll.c_str(),ll.c_str()));
        out += ll;
      }
    }
  }
}


void ProcessBalance(TMODINTF *i)
{
  CHTML out(i,L"Clients Balance");

  {
    CReadDBTable db(L"SELECT dev_id,srv_time_s,ussd FROM TBalance ORDER BY dev_id");

    CTable tbl(i);

    {
      CTableRow r(i);
      { CTableCellHeader c(i); CUnderline u(i); out += HTMLFilter("DEV_ID"); }
      { CTableCellHeader c(i); out += HTMLFilter("server time (local)"); }
      { CTableCellHeader c(i); out += HTMLFilter("USSD answer"); }
    }

    while ( db.FetchRow() )
    {
      CTableRow r(i);
      { CTableCell c(i); out += db.GetAsInt(0); }
      { CTableCell c(i); out += HTMLFilter(db.GetAsText(1)); }
      { CTableCell c(i); out += HTMLFilter(db.GetAsText(2)); }
    }
  }
}


void ProcessSMS(TMODINTF *i)
{
  std::string sms = i->_POST("sms");

  if ( !sms.empty() )
     {
       TCFG cfg;
       ReadConfig(cfg);

       WSADATA winsockdata;
       WSAStartup(MAKEWORD(2,2),&winsockdata);

       SOCKET h_socket_udp = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

       struct sockaddr_in sin;
       sin.sin_family = AF_INET;
       sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
       sin.sin_port = htons(cfg.port_udp);

       BOOL sent_ok = (sendto(h_socket_udp,sms.data(),sms.size(),0,(const struct sockaddr*)&sin,sizeof(sin)) == sms.size());
       int err = WSAGetLastError();

       FinishSocket(h_socket_udp);
       
       WSACleanup();
       
       i->Echo(sent_ok?"OK":CFormat("UDP send failed %d",err));
     }
  else
     {
       i->Echo("Empty string");
     }
}


void __stdcall Processing(TMODINTF *i)
{
  std::string action = i->_REQUEST("action");

  if ( action == "log" )
     {
       ProcessLog(i);
     }
  else
  if ( action == "ping" )
     {
       ProcessPing(i);
     }
  else
  if ( action == "balance" )
     {
       ProcessBalance(i);
     }
  else
  if ( action == "sms" )
     {
       ProcessSMS(i);
     }
  else
     {
       i->Echo("What do you mean?");
     }
}


