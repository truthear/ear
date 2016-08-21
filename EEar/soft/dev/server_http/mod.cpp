
#include "include.h"



HINSTANCE g_instance = NULL;


struct TGeoLabel 
{
  double lat,lon;
  std::string label;
  TGeoLabel(double _lat,double _lon,const std::string& _label) : lat(_lat), lon(_lon), label(_label) {}
};

typedef std::vector<TGeoLabel> TGeoLabels;


void ProduceGoogleMap(TMODINTF *i,const WCHAR *title,const TGeoLabels& labels)
{
  CHTML out(i,title,"onload=\"Init()\"");

  out.AddRawString(LoadRawResource(g_instance,IDR_MAPBEGIN));

  for ( unsigned n = 0; n < labels.size(); n++ )
      {
        const TGeoLabel& l = labels[n];

        std::string s = l.label;
        s = ReplaceSymbol(s,'\"','\'');
        s = ReplaceSymbol(s,'\\','/');
        s = ReplaceSymbol(s,'\r',' ');
        s = ReplaceSymbol(s,'\n',' ');
        s = ReplaceSymbol(s,'\t',' ');
        
        out += CFormat("%c[%.7f,%.7f,\"%s\"]\n",n==0?' ':',',l.lat,l.lon,s.c_str());
      }

  out.AddRawString(LoadRawResource(g_instance,IDR_MAPEND));
}




void ProcessLog(TMODINTF *i)
{
  CReadDBTable db(L"SELECT ev_time,ev_desc FROM TLog ORDER BY ev_time DESC LIMIT 5000");

  {
    CHTML out(i,L"Server Log");

    CPre pre(i);

    while ( db.FetchRow() )
    {
      out += HTMLFilter("["+OurTimeToString(db.GetAsInt64(0))+"] ");
      out += HTMLFilter(db.GetAsText(1));
      out += "\n";
    }
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
  else
     {
       TGeoLabels ar;
     
       while ( db.FetchRow() )
       {
         ar.push_back(TGeoLabel(db.GetAsDouble(7),db.GetAsDouble(8),(const char*)CFormat("%d/%d [%ws] %ws",db.GetAsInt(0),db.GetAsInt(1),db.GetAsText(5).c_str(),db.GetAsText(6).c_str())));
       }

       ProduceGoogleMap(i,title,ar);
     }
}


void ProcessBalance(TMODINTF *i)
{
  CReadDBTable db(L"SELECT dev_id,srv_time_s,ussd FROM TBalance ORDER BY dev_id");

  {
    CHTML out(i,L"Clients Balance");

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


void ProcessFDetect(TMODINTF *i)
{
  CReadDBTable db(L"SELECT dev_id,sector,cl_ver,fd_ver,event_time_utc,srv_time_lcl,lat,lon,is_from_sms FROM TFDetect ORDER BY srv_time_lcl DESC LIMIT 5000");

  {
    CHTML out(i,L"Single f-detect results");

    CTable tbl(i);

    {
      CTableRow r(i);
      { CTableCellHeader c(i); out += HTMLFilter("dev_id"); }
      { CTableCellHeader c(i); out += HTMLFilter("sector"); }
      { CTableCellHeader c(i); out += HTMLFilter("version"); }
      { CTableCellHeader c(i); out += HTMLFilter("event time (UTC)"); }
      { CTableCellHeader c(i); CUnderline u(i); out += HTMLFilter("SERVER TIME (LOCAL)"); }
      { CTableCellHeader c(i); out += HTMLFilter("geo location"); }
      { CTableCellHeader c(i); out += HTMLFilter("from SMS?"); }
    }

    while ( db.FetchRow() )
    {
      CTableRow r(i);
      { CTableCell c(i); out += db.GetAsInt(0); }
      { CTableCell c(i); out += db.GetAsInt(1); }
      { CTableCell c(i); out += HTMLFilter(CFormat("v%X (%X)",db.GetAsInt(2),db.GetAsInt(3))); }
      { CTableCell c(i); out += HTMLFilter(OurTimeToString(db.GetAsInt64(4))+" UTC"); }
      { CTableCell c(i); out += HTMLFilter(OurTimeToString(db.GetAsInt64(5))); }
      { 
        CTableCell c(i); 
        std::string ll = CFormat("%.7f,%.7f",db.GetAsDouble(6),db.GetAsDouble(7));
        CAnchor a(i,CFormat("https://maps.google.com/maps?ll=%s&spn=0.001,0.001&t=m&q=%s",ll.c_str(),ll.c_str()));
        out += ll;
      }
      { CTableCell c(i); out += db.GetAsInt(8)?"YES":"-"; }
    }
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
         { CTableCellHeader c(i); out += HTMLFilter("sector"); }
         { CTableCellHeader c(i); CUnderline u(i); out += HTMLFilter("TIME (UTC)"); }
         { CTableCellHeader c(i); out += HTMLFilter("geo location"); }
         { CTableCellHeader c(i); out += HTMLFilter("sources"); }
       }

       while ( db.FetchRow() )
       {
         CTableRow r(i);
         { CTableCell c(i); out += db.GetAsInt(0); }
         { CTableCell c(i); out += HTMLFilter(OurTimeToString(db.GetAsInt64(1))+" UTC"); }
         { 
           CTableCell c(i); 
           std::string ll = CFormat("%.7f,%.7f",db.GetAsDouble(2),db.GetAsDouble(3));
           CAnchor a(i,CFormat("https://maps.google.com/maps?ll=%s&spn=0.001,0.001&t=m&q=%s",ll.c_str(),ll.c_str()));
           out += ll;
         }
         { CTableCell c(i); out += db.GetAsInt(4); }
       }
     }
  else
     {
       TGeoLabels ar;
     
       while ( db.FetchRow() )
       {
         ar.push_back(TGeoLabel(db.GetAsDouble(2),db.GetAsDouble(3),OurTimeToString(db.GetAsInt64(1))+" UTC"));
       }

       ProduceGoogleMap(i,title,ar);
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


extern "C" BOOL WINAPI DllMain(HINSTANCE hInst,int reason,void *lpReserved)
{
  if ( reason == DLL_PROCESS_ATTACH )
     {
       g_instance = hInst;
     }
  else
  if ( reason == DLL_PROCESS_DETACH )
     {
     }

  return TRUE;
}


