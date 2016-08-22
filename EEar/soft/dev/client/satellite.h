
#ifndef __SATELLITE_H__
#define __SATELLITE_H__


class CBoardGPS;


// this class uses CSysTicks which should be initialized first!
class CSatellite
{
          static const unsigned MAX_GPS_NONSYNC_INTERVAL = 120000;  // if gps_fix not happens after this time (in ms), then we threat result as not-valid
          static const unsigned MAX_TS_NONSYNC_INTERVAL = 60000;  // if time sync not happens after this time (in ms), then we threat result as not-valid
          
          struct {
          CCriticalSection cs;
          double lat;
          double lon;
          short gnss;
          unsigned last_fix_time;  // CSysTicks::GetCounter() at moment of gps_fix event
          } m_gps;

          struct {
          CCriticalSection cs;
          OURTIME ts_value;        // TS value (unshifted) at moment of TS
          unsigned ts_ticks;       // CSysTicks::GetCounter() at moment of TS
          OURTIME shift;           // calculated shift value for RTC::SetShift()
          unsigned last_sync_time; // CSysTicks::GetCounter() at moment of shift was set
          } m_time;

          bool b_use_gps;
          bool b_use_glonass;
          bool b_use_galileo;
          bool b_use_beidou;

          bool b_device_initialized;

          unsigned m_starttime;

          CBoardGPS *p_gps;

  public:
          CSatellite(int rate,bool use_gps=true,bool use_glonass=true,bool use_galileo=false,bool use_beidou=false);
          ~CSatellite();

          void Poll();

          // WARNING!!! do not call these two functions no-stop all the time, because of critical sections used, probably no time for IRQ will remain!
          // it is better to poll 1 time per sec,
          // if functions returns false then returned data can be used on your own risk, 
          // it can be unset (on startup), or expired (when gps_fix or sync happens many time ago)
          bool GetNavData(double& _lat,double& _lon,short& _gnss);
          bool GetTimeData(OURTIME& _shift);

          unsigned GetLastFixTime();  // last time of gps_fix, for debug only
          unsigned GetLastSyncTime();  // last time of ts sync, for debug only
          
          // callback for RTC TimeStamp (PPS pulse)
          void OnTS(OURTIME ts_unshifted);

  private:
          static void OnNMEAWrapper(void*,OURTIME _time,double _lat,double _lon,short _gnss);
          void OnNMEA(OURTIME _time,double _lat,double _lon,short _gnss);
};



#endif
