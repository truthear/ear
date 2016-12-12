
#include "include.h"



HINSTANCE g_instance = NULL;



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

