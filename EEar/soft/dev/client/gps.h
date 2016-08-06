
#ifndef __GPS_H__
#define __GPS_H__


class CUART;


// Note! All NMEA commands interpreted here as string between '$' and '*' not including these symbols!
class CGPS
{
  public:
          typedef void (*TCALLBACK)(void*,const char *nmea,unsigned str_len);

  private:        
          static const unsigned NMEA_MAX = 82;  // defined by standard
          
          TCALLBACK p_cb;
          void *p_cbparm;

          char s_cmd[NMEA_MAX];  // not null-terminated!
          unsigned m_pos;  // current recv symbol idx

          CUART *p_uart;

  public:
          CGPS(EBoardUarts uart,int rate,TCALLBACK cb,void *cbparm,int irq_priority);
          virtual ~CGPS();

          void SendNMEA(const char *nmea);

  private:
          static void UARTCallbackWrapper(void*,unsigned char data);
          void UARTCallback(unsigned char data);
          static bool IsEquCRC(unsigned char crc,char cmp_hi,char cmp_lo);
};


class CTelitGPS : public CGPS
{
  public:
          CTelitGPS(EBoardUarts uart,int rate,TCALLBACK cb,void *cbparm,int irq_priority)
            : CGPS(uart,rate,cb,cbparm,irq_priority) {}

          void EnableOnlyRMC();
          bool ParseNMEA(const char *nmea,unsigned str_len,char *_yymmddhhnnsssss=NULL,double *_lat=NULL,double *_lon=NULL);

  private:
          static bool IsLenOk(const char *s,int v_min,int v_max);
          static double ConvertLatLon(const char *s,int numdegree,bool negative);
};


class CBoardGPS : public CTelitGPS
{
  public:
          typedef void (*TCALLBACK)(void*,OURTIME _time,double _lat,double _lon);

  private:
          static TCALLBACK p_cb;  // we use static because initially cb must be NULL (callback from upper class can be initiated before constructor of this class - in theory :)
          static void *p_cbparm;

  public:
          CBoardGPS(int rate,TCALLBACK cb,void *cbparm=NULL,int irq_priority=5);

  private:
          static void OnNMEAWrapper(void *parm,const char *nmea,unsigned str_len);
          void OnNMEA(const char *nmea,unsigned str_len);
          static int ParseInt(const char *s,int idx_from,int idx_to);

};


#endif
