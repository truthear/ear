
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


}


void ProcessBalance(TMODINTF *i)
{
  COutput out(i,L"Clients USSD-Balance");


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
     {
       i->Echo("What do you mean?");
     }
}


