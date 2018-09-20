
#include <windows.h>
#include <shlwapi.h>
#include <objbase.h>
#include <locationapi.h>
#include <stdio.h>
#include <stdarg.h>
#include <conio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <string>


#define SAFERELEASE(obj)  { if ( obj ) obj->Release(); obj = NULL; }
#define MAX(a,b)   ((a)>(b)?(a):(b))
#define MIN(a,b)   ((a)<(b)?(a):(b))


class CGeoLocation
{
          BOOL b_unsupported;
          
          volatile BOOL b_data_valid;
          volatile double m_lat;
          volatile double m_lon;

          unsigned m_last_poll_time;

          HANDLE h_event_stop;
          HANDLE h_thread;
          DWORD m_thread_id;

  public:
          CGeoLocation();
          ~CGeoLocation();

          BOOL GetData(double& _lat,double& _lon) const;

  private:
          static DWORD WINAPI ThreadProcWrapper(void *parm);
          void ThreadProc();
          void Poll(ILocation* &pl,ILocationEvents *events);

  protected:
          friend class CLocEvents;
          void OnLocationChanged(double lat,double lon);
};



class CLocEvents : public ILocationEvents
{
          CGeoLocation *p_host;

  public:
          CLocEvents(CGeoLocation *_host) : p_host(_host) {}

          // IUnknown
          STDMETHODIMP_(ULONG) AddRef() 
          { 
            return 2;
          }
          
          STDMETHODIMP_(ULONG) Release() 
          { 
            return 1;
          }

          STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
          {
            if (NULL == ppvObject) return E_POINTER;
            if (riid == __uuidof(IUnknown))
            {
                *ppvObject = static_cast<IUnknown*>(this);
                AddRef();
                return S_OK;
            }
            if (riid == __uuidof(ILocationEvents))
            {
                *ppvObject = static_cast<ILocationEvents*>(this);
                AddRef();
                return S_OK;
            }
            *ppvObject = NULL;
            return E_NOINTERFACE;
          }

          // ILocationEvents
          STDMETHODIMP OnLocationChanged(REFIID reportType,ILocationReport *pLocationReport)
          {
            if ( reportType == IID_ILatLongReport && pLocationReport )
               {
                 ILatLongReport *pll = NULL;
                 pLocationReport->QueryInterface(IID_ILatLongReport,(void**)&pll);
                 if ( pll )
                    {
                      double lat=0,lon=0;
                      if ( pll->GetLatitude(&lat) == S_OK && pll->GetLongitude(&lon) == S_OK )
                         {
                           p_host->OnLocationChanged(lat,lon);
                         }

                      pll->Release();
                    }
               }
            
            return S_OK;
          }
          
          STDMETHODIMP OnStatusChanged(REFIID reportType,LOCATION_REPORT_STATUS newStatus)
          {
            return S_OK;
          }
};


//////////////////////


CGeoLocation::CGeoLocation()
{
  b_unsupported = FALSE;
  b_data_valid = FALSE;
  m_lat = 0;
  m_lon = 0;

  m_last_poll_time = GetTickCount() - 10000;

  h_event_stop = CreateEvent(NULL,FALSE,FALSE,NULL);
  h_thread = CreateThread(NULL,0,ThreadProcWrapper,this,0,&m_thread_id);
}


CGeoLocation::~CGeoLocation()
{
  SetEvent(h_event_stop);
  if ( WaitForSingleObject(h_thread,500) == WAIT_TIMEOUT )
     TerminateThread(h_thread,0);
  
  CloseHandle(h_thread);
  h_thread = NULL;

  CloseHandle(h_event_stop);
  h_event_stop = NULL;
}


DWORD WINAPI CGeoLocation::ThreadProcWrapper(void *parm)
{
  reinterpret_cast<CGeoLocation*>(parm)->ThreadProc();
  return 1;
}


void CGeoLocation::ThreadProc()
{
  CoInitialize(0);

  {
    CLocEvents events(this);

    ILocation *pl = NULL;

    while ( 1 )
    {
      DWORD wc = MsgWaitForMultipleObjects(1,&h_event_stop,FALSE,50,QS_ALLINPUT);
      
      if ( wc == WAIT_OBJECT_0 )
         break;

      if ( wc != WAIT_TIMEOUT )
         {
           MSG msg;
           while ( PeekMessage(&msg,NULL,0,0,PM_REMOVE) )
           {
             DispatchMessage(&msg);
           }
         }

      Poll(pl,&events);
    }

    SAFERELEASE(pl);
  }

  CoUninitialize();
}


void CGeoLocation::Poll(ILocation* &pl,ILocationEvents *events)
{
  if ( GetTickCount() - m_last_poll_time > 1000 )
     {
       if ( !pl )
          {
            if ( !b_unsupported )
               {
                 CoCreateInstance(CLSID_Location,NULL,CLSCTX_INPROC_SERVER,IID_ILocation,(void**)&pl);
               }

            if ( !pl )
               {
                 b_unsupported = TRUE;  // XP or server OS
               }
            else
               {
                 pl->RegisterForReport(events,IID_ILatLongReport,0);  // here OnStatusChanged() can be called immediately
                                                                      // we do not check return code
               }
          }

       m_last_poll_time = GetTickCount();
     }
}


void CGeoLocation::OnLocationChanged(double lat,double lon)
{
  m_lat = lat;
  m_lon = lon;
  b_data_valid = TRUE;
}


BOOL CGeoLocation::GetData(double& _lat,double& _lon) const
{
  _lat = m_lat;
  _lon = m_lon;

  return b_data_valid;
}


///////////

FILE *g_log = NULL;


char* GetFileNameInLocalAppDir(const char *local,char *out)
{
  char s[MAX_PATH] = "";
  GetModuleFileName(GetModuleHandle(NULL),s,sizeof(s));
  PathRemoveFileSpec(s);
  PathAppend(s,local);
  lstrcpy(out,s);
  return out;
}


char* GetLogFileName(char *out)
{
  char s[MAX_PATH] = "";
  GetModuleFileName(GetModuleHandle(NULL),s,sizeof(s));
  PathRemoveExtension(s);
  lstrcat(s,".txt");
  lstrcpy(out,s);
  return out;
}


void Add2Log(const char *fmt,...)
{
  char buffer[1024];
  va_list args;
  va_start(args,fmt);
  vsprintf(buffer,fmt,args);
  va_end(args);

  printf("%s",buffer);
  if ( g_log )
   fprintf(g_log,"%s",buffer);
}


void FlushLog()
{
  fflush(stdout);
  if ( g_log )
   fflush(g_log);
}



void PrintCommState(DCB dcb)
{
    //  Print some of the DCB structure values
    Add2Log( "\nBaudRate = %d, ByteSize = %d, Parity = %d, StopBits = %d\n", 
              dcb.BaudRate, 
              dcb.ByteSize, 
              dcb.Parity,
              dcb.StopBits );
}


std::string GetCurrentTimeStr()
{
  SYSTEMTIME st;
  GetLocalTime(&st);

  char s[100];
  sprintf(s,"%02d:%02d:%02d",st.wHour,st.wMinute,st.wSecond);
  return s;
}


static const unsigned char char2hex_t[256] =
{
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};


std::string Cmd(HANDLE h,const char *cmd,BOOL need_decode_answer=FALSE)
{
  std::string out;
  
  if ( cmd )
   Add2Log("> %s\n",cmd);
  
  DWORD wb = 0;
  if ( !cmd || (WriteFile(h,cmd,lstrlen(cmd),&wb,NULL) && wb == lstrlen(cmd) && WriteFile(h,"\r\n",2,&wb,NULL) && wb == 2) )
     {
       std::string rs;
       
       while (1)
       {
         char c;
         DWORD rb = 0;
         
         if ( ReadFile(h,&c,1,&rb,NULL) && rb == 1 )
            {
              if ( need_decode_answer )
                 {
                   if ( ((unsigned char)c >= '0' && (unsigned char)c <= '9') || ((unsigned char)c >= 'A' && (unsigned char)c <= 'F') )
                      {
                        rs += c;
                      }
                 }
              
              if ( c == '\r' )
                 {
                   Add2Log(" [%s]",GetCurrentTimeStr().c_str());
                 }
              
              Add2Log("%c",c);
              //fflush(stdout);

              if ( c == '\n' )
              {
                if ( need_decode_answer && !rs.empty() )
                   {
                     if ( (rs.size() % 2) == 0 )
                        {
                          for ( int n = 0; n < rs.size()/2; n++ )
                              {
                                int symb = ((int)char2hex_t[(unsigned char)rs[n*2+0]] << 4) | (int)char2hex_t[(unsigned char)rs[n*2+1]];
                                Add2Log("%c",symb);
                                out += symb;
                              }
                          Add2Log("\n");
                        }
                   }
                
                break;
              }
            }
         else
            {
              Add2Log("\nRead error\n");
              break;
            }
       }
     }
  else
   Add2Log("Write failed\n");

  FlushLog();

  return out;
}


void UpdateLatLon(const CGeoLocation& geo,const char *s_ini,double& _lat,double& _lon)
{
  char s_lat[100] = "";
  GetPrivateProfileString("Main","Lat","",s_lat,sizeof(s_lat),s_ini);
  char s_lon[100] = "";
  GetPrivateProfileString("Main","Lon","",s_lon,sizeof(s_lon),s_ini);

  if ( s_lat[0] && s_lon[0] )
     {
       sscanf(s_lat,"%lf",&_lat);
       sscanf(s_lon,"%lf",&_lon);
     }
  else
     {
       if ( !geo.GetData(_lat,_lon) )
          {
            _lat = 0;
            _lon = 0;
          }
     }

  Add2Log("Location: %.6f, %.6f\n",_lat,_lon);
  FlushLog();
}


#define EARTH_RADIUS_METERS  6371000.0

static double d2r(double d) { return d*(M_PI/180.0); }

double mdist(double s1,double d1,double s2,double d2)
{
  double t = sin(s1)*sin(s2)+cos(s1)*cos(s2)*cos(d2-d1);
  t = MIN(t,+1);
  t = MAX(t,-1);
  return EARTH_RADIUS_METERS*acos(t);
}


void PrintDistance(const std::string& s,double my_lat,double my_lon)
{
  if ( s.size() == 50 )  // should be changed on sender too!!!!
     {
       if ( fabs(my_lat) > 0.00001 && fabs(my_lon) > 0.00001 )
          {
            std::string cmd = s;

            for ( unsigned n = 0; n < cmd.size(); n++ )
                {
                  if ( cmd[n] == '_' )
                   cmd[n] = ' ';
                }

            int packet_num,errors;
            char s_khz[100],s_sf[100];
            double lat=0,lon=0;
            sscanf(cmd.c_str(),"%d %d %s %s %lf %lf",&packet_num,&errors,s_khz,s_sf,&lat,&lon);

            if ( fabs(lat) > 0.00001 && fabs(lon) > 0.00001 )
               {
                 double dist_m = mdist(d2r(my_lat),d2r(my_lon),d2r(lat),d2r(lon));

                 Add2Log("Distance: %d meters\n",(int)dist_m);
                 FlushLog();
               }
          }
     }
}




void main()
{
  char s_ini[MAX_PATH];
  GetFileNameInLocalAppDir("config.ini",s_ini);

  char s_port[100] = "";
  GetPrivateProfileString("Main","Port","",s_port,sizeof(s_port),s_ini);

  if ( !s_port[0] )
     {
       printf("No COM-port specified in config.ini!\n");
       return;
     }
  
  char s_log[MAX_PATH] = "";
  GetLogFileName(s_log);
  
  g_log = fopen(s_log,"at");
  if ( !g_log )
     {
       printf("error creating log file!\n");
       return;
     }
  
  char s_comm[MAX_PATH];
  sprintf(s_comm,"\\\\.\\%s",s_port);
  
  HANDLE h = CreateFile(s_comm,GENERIC_READ|GENERIC_WRITE,0/*FILE_SHARE_READ|FILE_SHARE_WRITE*/,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if ( h != INVALID_HANDLE_VALUE )
     {
       CGeoLocation geo;
       
       DCB dcb;
       ZeroMemory(&dcb,sizeof(dcb));
       dcb.DCBlength = sizeof(DCB);

       GetCommState(h,&dcb);
       dcb.BaudRate = 57600;
       SetCommState(h,&dcb);
       ZeroMemory(&dcb,sizeof(dcb));
       dcb.DCBlength = sizeof(DCB);
       GetCommState(h,&dcb);
       //PrintCommState(dcb);


       Cmd(h,"mac pause");
       Cmd(h,"radio set mod lora");
       Cmd(h,"radio set freq 868000000");
#ifdef MODE125SF7
       Cmd(h,"radio set bw 125");
       Cmd(h,"radio set sf sf7");
#endif
#ifdef MODE125SF10
       Cmd(h,"radio set bw 125");
       Cmd(h,"radio set sf sf10");
#endif
#ifdef MODE125SF12
       Cmd(h,"radio set bw 125");
       Cmd(h,"radio set sf sf12");
#endif
#ifdef MODE500SF7
       Cmd(h,"radio set bw 500");
       Cmd(h,"radio set sf sf7");
#endif
#ifdef MODE500SF10
       Cmd(h,"radio set bw 500");
       Cmd(h,"radio set sf sf10");
#endif
#ifdef MODE500SF12
       Cmd(h,"radio set bw 500");
       Cmd(h,"radio set sf sf12");
#endif
       Cmd(h,"radio set prlen 8");
       Cmd(h,"radio set crc on");
       Cmd(h,"radio set iqi off");
       Cmd(h,"radio set cr 4/5");
       Cmd(h,"radio set wdt 0");
       Cmd(h,"radio set sync 12");

       while (!kbhit())
       {
         double lat=0,lon=0;
         UpdateLatLon(geo,s_ini,lat,lon);
         Cmd(h,"mac pause");
         Cmd(h,"radio rx 0");
         std::string res = Cmd(h,NULL,TRUE);
         PrintDistance(res,lat,lon);
         Cmd(h,"radio get snr");
       }


       CloseHandle(h);
     }
  else
    Add2Log("Error opening port\n");

  fclose(g_log);
}



