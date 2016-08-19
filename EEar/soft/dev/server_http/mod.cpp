
#include "include.h"




void ProcessLog(TMODINTF *i)
{
  COutput out(i,L"Server Log");

  out += "<pre>\n";

  {
    CLocalDB sql(TRUE);

    CSQLiteQuery *q = sql.CreateQuery(L"SELECT ev_time,ev_desc FROM TLog ORDER BY ev_time DESC LIMIT 50000");

    BOOL is_data = FALSE;
    while ( q->Step(&is_data) && is_data )
    {
      out += HTMLFilter("["+OurTimeToString(q->GetAsInt64(0))+"] ");
      out += HTMLFilter(q->GetAsText(1));
      out += "\n";
    }

    q->Destroy();
  }

  out += "</pre>\n";
}


void ProcessPing(TMODINTF *i)
{
  COutput out(i,L"Clients Ping");

  {
    CLocalDB sql(TRUE);

    CSQLiteQuery *q = sql.CreateQuery(L"SELECT dev_id,sector,cl_ver,fd_ver,last_ts_ms,cl_time_s,srv_time_s,lat,lon FROM TPing ORDER BY dev_id");

    out += "<table border=\"1\" cellspacing=\"0\" cellpadding=\"3\">\n";

    out += "<tr>"
           "<td align=\"center\"><b>dev_id</b></td>"
           "<td align=\"center\"><b>sector</b></td>"
           "<td align=\"center\"><b>version</b></td>"
           "<td align=\"center\"><b>last_ts (msec)</b></td>"
           "<td align=\"center\"><b>client time (UTC)</b></td>"
           "<td align=\"center\"><b>server time (local)</b></td>"
           "<td align=\"center\"><b>geo location</b></td>"
           "</tr>\n";

    BOOL is_data = FALSE;
    while ( q->Step(&is_data) && is_data )
    {
      out += "<tr>\n";

      out += "<td>\n";
      out += HTMLFilter(CFormat("%d",q->GetAsInt(0)));
      out += "</td>\n";
      
      out += "<td>\n";
      out += HTMLFilter(CFormat("%d",q->GetAsInt(1)));
      out += "</td>\n";
      
      out += "<td>\n";
      out += HTMLFilter(CFormat("v%X (%X)",q->GetAsInt(2),q->GetAsInt(3)));
      out += "</td>\n";
      
      out += "<td>\n";
      out += HTMLFilter(CFormat("%d",q->GetAsInt(4)));
      out += "</td>\n";

      out += "<td>\n";
      out += HTMLFilter(q->GetAsText(5));
      out += "</td>\n";
      
      out += "<td>\n";
      out += HTMLFilter(q->GetAsText(6));
      out += "</td>\n";

      out += "<td>\n";
      out += CFormat("<a href=\"https://maps.google.com/maps?ll=%.7f,%.7f&spn=0.001,0.001&t=m&q=%.7f,%.7f\" target=\"_blank\">%.7f,%.7f</a>",q->GetAsDouble(7),q->GetAsDouble(8),q->GetAsDouble(7),q->GetAsDouble(8),q->GetAsDouble(7),q->GetAsDouble(8));
      out += "</td>\n";

      out += "</tr>\n";
    }

    out += "</table>\n";

    q->Destroy();
  }
}


void ProcessBalance(TMODINTF *i)
{
  COutput out(i,L"Clients USSD-Balance");

  {
    CLocalDB sql(TRUE);

    CSQLiteQuery *q = sql.CreateQuery(L"SELECT dev_id,srv_time_s,ussd FROM TBalance ORDER BY dev_id");

    out += "<table border=\"1\" cellspacing=\"0\" cellpadding=\"3\">\n";

    out += "<tr><td align=\"center\"><b>dev_id</b></td><td align=\"center\"><b>server time</b></td><td align=\"center\"><b>ussd</b></td></tr>\n";

    BOOL is_data = FALSE;
    while ( q->Step(&is_data) && is_data )
    {
      out += "<tr>\n";

      out += "<td>\n";
      out += HTMLFilter(CFormat("%d",q->GetAsInt(0)));
      out += "</td>\n";
      
      out += "<td>\n";
      out += HTMLFilter(q->GetAsText(1));
      out += "</td>\n";
      
      out += "<td>\n";
      out += HTMLFilter(q->GetAsText(2));
      out += "</td>\n";

      out += "</tr>\n";
    }

    out += "</table>\n";

    q->Destroy();
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


extern "C"
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


