
#include "include.h"



extern "C"
void __stdcall Processing(TMODINTF *i)
{
  i->Header("Expires","Thu, 19 Nov 1981 08:52:00 GMT",TRUE);
  i->Header("Cache-Control","no-store, no-cache, must-revalidate, post-check=0, pre-check=0",TRUE);
  i->Header("Pragma","no-cache",TRUE);

  std::string action = i->_REQUEST("action");

  if ( action == "log" )
     {

     }
  else
  if ( action == "ping" )
     {

     }
  else
  if ( action == "balance" )
     {

     }
  else
     {
       i->Echo("What do you mean?");
     }
}


