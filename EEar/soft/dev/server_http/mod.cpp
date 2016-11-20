
#include "include.h"




void ProcessLog(TMODINTF *i)
{
  CReadDBTable db(L"SELECT ev_time,ev_desc FROM TLog ORDER BY ev_time DESC LIMIT 5000");

  CHTML out(i,L"Server Log");

  CPre pre(i);

  while ( db.FetchRow() )
  {
    out += CHTMLTools::Filter("["+OurTimeToString(db.GetAsInt64(0))+"] ");
    out += CHTMLTools::Filter(db.GetAsText(1));
    out += "\n";
  }
}


void ProcessPing(TMODINTF *i,BOOL map_view)
{
  const WCHAR *title = L"Clients Ping";
  
  CReadDBTable db(L"SELECT dev_id,sector,cl_ver,fd_ver,last_ts_ms,cl_time_s,srv_time_s,lat,lon FROM TPing ORDER BY srv_time_lcl DESC");

  if ( !map_view )
     {
       CHTML out(i,title);

       CTable tbl(i);

       {
         CTableRow r(i);
         CHTMLTools::ProduceHeaderCell(i,out,"dev_id");
         CHTMLTools::ProduceHeaderCell(i,out,"sector");
         CHTMLTools::ProduceHeaderCell(i,out,"version");
         CHTMLTools::ProduceHeaderCell(i,out,"last_ts (msec)");
         CHTMLTools::ProduceHeaderCell(i,out,"client time (UTC)");
         CHTMLTools::ProduceHeaderCell(i,out,"SERVER TIME (LOCAL)",TRUE);
         CHTMLTools::ProduceHeaderCell(i,out,"geo location");
       }

       while ( db.FetchRow() )
       {
         CTableRow r(i);
         CHTMLTools::ProduceIntCell(i,out,db,0);
         CHTMLTools::ProduceIntCell(i,out,db,1);
         CHTMLTools::ProduceVerCell(i,out,db,2,3);
         CHTMLTools::ProduceIntCell(i,out,db,4);
         CHTMLTools::ProduceTextCell(i,out,db,5);
         CHTMLTools::ProduceTextCell(i,out,db,6);
         CHTMLTools::ProduceGeoCell(i,out,db,7,8);
       }
     }
  else
     {
       CGoogleMap gm(i,title);
     
       while ( db.FetchRow() )
       {
         gm.Add(db.GetAsDouble(7),db.GetAsDouble(8),(const char*)CFormat("%d/%d [%ws] %ws",db.GetAsInt(0),db.GetAsInt(1),db.GetAsText(5).c_str(),db.GetAsText(6).c_str()));
       }
     }
}


void ProcessBalance(TMODINTF *i)
{
  CReadDBTable db(L"SELECT dev_id,srv_time_s,ussd FROM TBalance ORDER BY dev_id");

  CHTML out(i,L"Clients Balance");

  CTable tbl(i);

  {
    CTableRow r(i);
    CHTMLTools::ProduceHeaderCell(i,out,"DEV_ID",TRUE);
    CHTMLTools::ProduceHeaderCell(i,out,"server time (local)");
    CHTMLTools::ProduceHeaderCell(i,out,"USSD answer");
  }

  while ( db.FetchRow() )
  {
    CTableRow r(i);
    CHTMLTools::ProduceIntCell(i,out,db,0);
    CHTMLTools::ProduceTextCell(i,out,db,1);
    CHTMLTools::ProduceTextCell(i,out,db,2);
  }
}


void ProcessFDetect(TMODINTF *i)
{
  CReadDBTable db(L"SELECT dev_id,sector,cl_ver,fd_ver,event_time_utc,srv_time_lcl,lat,lon,len_ms,db_amp,is_from_sms FROM TFDetect ORDER BY srv_time_lcl DESC LIMIT 5000");

  CHTML out(i,L"Single f-detect results");

  CTable tbl(i);

  {
    CTableRow r(i);
    CHTMLTools::ProduceHeaderCell(i,out,"dev_id");
    CHTMLTools::ProduceHeaderCell(i,out,"sector");
    CHTMLTools::ProduceHeaderCell(i,out,"version");
    CHTMLTools::ProduceHeaderCell(i,out,"event time (UTC)");
    CHTMLTools::ProduceHeaderCell(i,out,"SERVER TIME (LOCAL)",TRUE);
    CHTMLTools::ProduceHeaderCell(i,out,"geo location");
    CHTMLTools::ProduceHeaderCell(i,out,"length (ms)");
    CHTMLTools::ProduceHeaderCell(i,out,"dB amp");
    CHTMLTools::ProduceHeaderCell(i,out,"from SMS?");
  }

  while ( db.FetchRow() )
  {
    CTableRow r(i);
    CHTMLTools::ProduceIntCell(i,out,db,0);
    CHTMLTools::ProduceIntCell(i,out,db,1);
    CHTMLTools::ProduceVerCell(i,out,db,2,3);
    CHTMLTools::ProduceTimeCell(i,out,db,4," UTC");
    CHTMLTools::ProduceTimeCell(i,out,db,5);
    CHTMLTools::ProduceGeoCell(i,out,db,6,7);
    CHTMLTools::ProduceIntCell(i,out,db,8);
    CHTMLTools::ProduceDoubleCell(i,out,db,9,1);
    CHTMLTools::ProduceIntCell(i,out,db,10);
  }
}


void ProcessResult(TMODINTF *i,BOOL map_view)
{
  const WCHAR *title = L"Results";
  
  CReadDBTable db(L"SELECT sector,time_utc,lat,lon,numsources FROM TResult ORDER BY time_utc DESC LIMIT 5000");

  if ( !map_view )
     {
       CHTML out(i,title);

       CTable tbl(i);

       {
         CTableRow r(i);
         CHTMLTools::ProduceHeaderCell(i,out,"sector");
         CHTMLTools::ProduceHeaderCell(i,out,"TIME (UTC)",TRUE);
         CHTMLTools::ProduceHeaderCell(i,out,"geo location");
         CHTMLTools::ProduceHeaderCell(i,out,"sources");
       }

       while ( db.FetchRow() )
       {
         CTableRow r(i);
         CHTMLTools::ProduceIntCell(i,out,db,0);
         CHTMLTools::ProduceTimeCell(i,out,db,1," UTC");
         CHTMLTools::ProduceGeoCell(i,out,db,2,3);
         CHTMLTools::ProduceIntCell(i,out,db,4);
       }
     }
  else
     {
       CGoogleMap gm(i,title);
     
       while ( db.FetchRow() )
       {
         gm.Add(db.GetAsDouble(2),db.GetAsDouble(3),OurTimeToString(db.GetAsInt64(1))+" UTC");
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


// DLL export function
void __stdcall Processing(TMODINTF *i)
{
  std::string action = i->_REQUEST("action");
  std::string map = i->_REQUEST("map");
  BOOL is_map = (map=="true");

  if ( action == "log" )
     {
       ProcessLog(i);
     }
  else
  if ( action == "ping" )
     {
       ProcessPing(i,is_map);
     }
  else
  if ( action == "balance" )
     {
       ProcessBalance(i);
     }
  else
  if ( action == "fd" )
     {
       ProcessFDetect(i);
     }
  else
  if ( action == "result" )
     {
       ProcessResult(i,is_map);
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


