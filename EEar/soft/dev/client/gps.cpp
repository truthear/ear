
#include "include.h"




CGPS::CGPS(EBoardUarts uart,int rate,TCALLBACK cb,void *cbparm,int irq_priority)
{
  p_cb = cb;
  p_cbparm = cbparm;

  m_pos = 0;

  // must be created at the end of constructor:
  p_uart = new CBoardUART(uart,rate,true,true,UARTCallbackWrapper,this,irq_priority);
}


CGPS::~CGPS()
{
  p_cb = NULL;

  delete p_uart;
  p_uart = NULL;
}


void CGPS::SendNMEA(const char *s)
{
  if ( s )
     {
       p_uart->SendByte('$');
       
       unsigned char crc = 0;

       while ( 1 )
       {
         unsigned char c = *s++;
         if ( c == 0 )
            {
              char t[16];
              sprintf(t,"%02X",(unsigned int)crc);

              p_uart->SendByte('*');
              p_uart->SendByte(t[0]);
              p_uart->SendByte(t[1]);
              p_uart->SendByte('\r');
              p_uart->SendByte('\n');
              break;
            }
         else
            {
              crc ^= c;
              p_uart->SendByte(c);
            }
       }
     }
}


void CGPS::UARTCallbackWrapper(void *parm,unsigned char data)
{
  reinterpret_cast<CGPS*>(parm)->UARTCallback(data);
}


void CGPS::UARTCallback(unsigned char data)
{
  if ( m_pos == NMEA_MAX )
     {
       m_pos = 0;
     }

  s_cmd[m_pos++] = (char)data;

  if ( data == '\n' )
     {
       if ( m_pos > 6 )
          {
            if ( s_cmd[0] == '$' && s_cmd[m_pos-2] == '\r' && s_cmd[m_pos-5] == '*' )
               {
                 // compute and compare crc
                 unsigned char crc = 0;
                 for ( unsigned n = 1; n < m_pos-5; n++ )
                     {
                       crc ^= s_cmd[n];
                     }

                 if ( IsEquCRC(crc,s_cmd[m_pos-4],s_cmd[m_pos-3]) )
                    {
                      if ( p_cb )
                         {
                           s_cmd[m_pos-5] = 0; // null terminator
                           p_cb(p_cbparm,s_cmd+1,m_pos-6);
                         }
                    }
               }
          }

       m_pos = 0;  // restart
     }
}


bool CGPS::IsEquCRC(unsigned char crc,char cmp_hi,char cmp_lo)
{
  static const char hx_u[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
  static const char hx_l[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

  unsigned crc_hi = (crc>>4)&0xF;
  unsigned crc_lo = crc&0xF;

  return (hx_u[crc_hi] == cmp_hi && hx_u[crc_lo] == cmp_lo) || (hx_l[crc_hi] == cmp_hi && hx_l[crc_lo] == cmp_lo);
}



void CTelitGPS::EnableOnlyRMC()
{
  SendNMEA("PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
}


bool CTelitGPS::ParseNMEA(const char *nmea,unsigned str_len,char *_yymmddhhnnsssss,double *_lat,double *_lon)
{
  bool rc = false;

  if ( nmea && str_len > 5 && str_len <= 76 )
     {
       if ( nmea[2] == 'R' && nmea[3] == 'M' && nmea[4] == 'C' )  // operate only with RMC command
          {
            if ( nmea[str_len-1] == 'A' || nmea[str_len-1] == 'D' )  // autonomous or differential modes only
               {
                 unsigned cnt = 0;
                 for ( unsigned n = 5; n < str_len; n++ )
                     {
                       if ( nmea[n] == ',' )
                          {
                            cnt++;
                          }
                     }

                 const unsigned NUM_FIELDS = 12;

                 if ( cnt == NUM_FIELDS )
                    {
                      char s[100];
                      strcpy(s,nmea);

                      const char* fields[NUM_FIELDS];

                      unsigned idx = 0;
                      for ( unsigned n = 5; n < str_len; n++ )
                          {
                            if ( s[n] == ',' )
                               {
                                 fields[idx++] = &s[n+1];
                                 s[n] = 0;
                               }
                          }

                      assert(idx == NUM_FIELDS);

                      if ( fields[1][0] == 'A' )  // data valid?
                         {
                           const char *p_time = fields[0];
                           const char *p_date = fields[8];
                           const char *p_lat = fields[2];
                           const char *p_lati = fields[3];
                           const char *p_lon = fields[4];
                           const char *p_loni = fields[5];
                           
                           if ( IsLenOk(p_time,10,10) && 
                                IsLenOk(p_date,6,6) && 
                                IsLenOk(p_lat ,7,16) && 
                                IsLenOk(p_lati,1,1) && 
                                IsLenOk(p_lon ,8,17) && 
                                IsLenOk(p_loni,1,1) )
                              {
                                rc = true;

                                if ( _yymmddhhnnsssss )
                                   {
                                     char *dst = _yymmddhhnnsssss;

                                     *dst++ = p_date[4];
                                     *dst++ = p_date[5];
                                     *dst++ = p_date[2];
                                     *dst++ = p_date[3];
                                     *dst++ = p_date[0];
                                     *dst++ = p_date[1];
                                     *dst++ = p_time[0];
                                     *dst++ = p_time[1];
                                     *dst++ = p_time[2];
                                     *dst++ = p_time[3];
                                     *dst++ = p_time[4];
                                     *dst++ = p_time[5];
                                     *dst++ = p_time[7];
                                     *dst++ = p_time[8];
                                     *dst++ = p_time[9];
                                     *dst++ = 0; // term
                                   }

                                if ( _lat )
                                   {
                                     *_lat = ConvertLatLon(p_lat,2,p_lati[0]=='S');
                                   }

                                if ( _lon )
                                   {
                                     *_lon = ConvertLatLon(p_lon,3,p_loni[0]=='W');
                                   }
                              }
                         }
                    }
               }
          }
     }

  return rc;
}


bool CTelitGPS::IsLenOk(const char *s,int v_min,int v_max)
{
  bool rc = false;

  if ( s )
     {
       int len = strlen(s);

       rc = (len >= v_min && len <= v_max);
     }

  return rc;
}


double CTelitGPS::ConvertLatLon(const char *s,int numdegree,bool negative)
{
  char t[8];

  CLEAROBJ(t);

  for ( int n = 0; n < numdegree; n++ )
      {
        t[n] = s[n];
      }

  int degree = atoi(t);
  double mins = atof(s+numdegree);
  double res = (double)degree+mins/60.0;

  if ( negative )
     {
       res = -res;
     }

  return res;
}



CBoardGPS::TCALLBACK CBoardGPS::p_cb = NULL;  // we use static because initially cb must be NULL (callback from upper class can be initiated before constructor of this class - in theory :)
void* CBoardGPS::p_cbparm = NULL;


CBoardGPS::CBoardGPS(int rate,TCALLBACK cb,void *cbparm,int irq_priority)
  : CTelitGPS(BOARD_UART_GPS,rate,OnNMEAWrapper,this,irq_priority) 
{
  p_cbparm = cbparm;
  p_cb = cb;
}


void CBoardGPS::OnNMEAWrapper(void *parm,const char *nmea,unsigned str_len)
{
  reinterpret_cast<CBoardGPS*>(parm)->OnNMEA(nmea,str_len);
}


void CBoardGPS::OnNMEA(const char *nmea,unsigned str_len)
{
  if ( p_cb )
     {
       char yymmddhhnnsssss[16];
       double lat,lon; 
        
       if ( ParseNMEA(nmea,str_len,yymmddhhnnsssss,&lat,&lon) )
          {
            int yy = ParseInt(yymmddhhnnsssss,0,1);
            int mm = ParseInt(yymmddhhnnsssss,2,3);
            int dd = ParseInt(yymmddhhnnsssss,4,5);
            int hh = ParseInt(yymmddhhnnsssss,6,7);
            int nn = ParseInt(yymmddhhnnsssss,8,9);
            int ss = ParseInt(yymmddhhnnsssss,10,11);
            int ms = ParseInt(yymmddhhnnsssss,12,14);

            OURTIME ourtime = ConvertOurTime(2000+yy,mm,dd,hh,nn,ss,ms);
            
            p_cb(p_cbparm,ourtime,lat,lon);
          }
     }
}


int CBoardGPS::ParseInt(const char *s,int idx_from,int idx_to)
{
  char t[16];

  char *dst = t;
  
  for ( int n = idx_from; n <= idx_to; n++ )
      {
        *dst++ = s[n];
      }

  *dst++ = 0;

  return atoi(t);
}








