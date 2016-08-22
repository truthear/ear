
#include "include.h"




CSatellite::CSatellite(int rate,bool use_gps,bool use_glonass,bool use_galileo,bool use_beidou)
{
  m_gps.lat = 0;
  m_gps.lon = 0;
  m_gps.gnss = 0x3f3f; // '??'
  m_gps.last_fix_time = CSysTicks::GetCounter()-MAX_GPS_NONSYNC_INTERVAL-1;

  m_time.ts_value = 0;
  m_time.ts_ticks = CSysTicks::GetCounter()-1001;
  m_time.shift = 0;
  m_time.last_sync_time = CSysTicks::GetCounter()-MAX_TS_NONSYNC_INTERVAL-1;

  b_use_gps = use_gps;
  b_use_glonass = use_glonass;
  b_use_galileo = use_galileo;
  b_use_beidou = use_beidou;

  b_device_initialized = false;

  m_starttime = CSysTicks::GetCounter();

  p_gps = new CBoardGPS(rate,OnNMEAWrapper,this);
}


CSatellite::~CSatellite()
{
  SAFEDELETE(p_gps);
}


void CSatellite::Poll()
{
  if ( !b_device_initialized )
     {
       if ( CSysTicks::GetCounter() - m_starttime > p_gps->GetWarmingUpTime() )
          {
            p_gps->EnableOnlyRMC();   // optimization
            CSysTicks::Delay(50);
            p_gps->SetSearchMode(b_use_gps,b_use_glonass,b_use_galileo,b_use_beidou);

            b_device_initialized = true;
          }
     }
}


bool CSatellite::GetNavData(double& _lat,double& _lon,short& _gnss)
{
  CCSGuard g(m_gps.cs);

  _lat = m_gps.lat;
  _lon = m_gps.lon;
  _gnss = m_gps.gnss;

  return CSysTicks::GetCounter() - m_gps.last_fix_time < MAX_GPS_NONSYNC_INTERVAL;
}


bool CSatellite::GetTimeData(OURTIME& _shift)
{
  CCSGuard g(m_time.cs);

  _shift = m_time.shift;

  return CSysTicks::GetCounter() - m_time.last_sync_time < MAX_TS_NONSYNC_INTERVAL;
}


unsigned CSatellite::GetLastFixTime()
{
  return m_gps.last_fix_time;  // no CS needed
}


unsigned CSatellite::GetLastSyncTime()
{
  return m_time.last_sync_time;  // no CS needed
}


void CSatellite::OnTS(OURTIME ts_unshifted)
{
  // no guard needed!
  
  m_time.ts_value = ts_unshifted;
  m_time.ts_ticks = CSysTicks::GetCounter();
}


void CSatellite::OnNMEAWrapper(void *parm,OURTIME _time,double _lat,double _lon,short _gnss)
{
  reinterpret_cast<CSatellite*>(parm)->OnNMEA(_time,_lat,_lon,_gnss);
}


void CSatellite::OnNMEA(OURTIME _time,double _lat,double _lon,short _gnss)
{
  // geo processing
  if ( m_gps.cs.IsUnlocked() )
     {
       m_gps.lat = _lat;
       m_gps.lon = _lon;
       m_gps.gnss = _gnss;
       m_gps.last_fix_time = CSysTicks::GetCounter();
     }

  // time sync processing
  if ( (_time % 1000) == 0 )  // operate only when 1-sec-aligned UTC time
     {
       if ( CSysTicks::GetCounter() - m_time.ts_ticks < 1000 )
          {
            if ( m_time.cs.IsUnlocked() )
               {
                 m_time.shift = _time - m_time.ts_value;
                 m_time.last_sync_time = CSysTicks::GetCounter();
               }
          }
     }
}



