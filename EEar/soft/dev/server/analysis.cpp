
#include "include.h"




#define EARTH_RADIUS_METERS  6371000.0
#define SOUND_SPEED          331.0  // m/s



// lat/lon (radians) <--> X/Y (meters) conversion
class CLLXYRad
{
          double m_lat0;
          double m_lon0;

  public:
          CLLXYRad(double lat0,double lon0) : m_lat0(lat0), m_lon0(lon0) {}

          void GetXY(double lat,double lon,double& _x,double& _y) const;
          void GetLL(double x,double y,double& _lat,double& _lon) const;

  private:
          static double mdist(double s1,double d1,double s2,double d2);
          static double sqr(double x) { return x*x; }
};


// lat/lon (degrees) <--> X/Y (meters) conversion
class CLLXYDeg : public CLLXYRad
{
  public:
          CLLXYDeg(double lat0,double lon0) : CLLXYRad(d2r(lat0),d2r(lon0)) {}

          void GetXY(double lat,double lon,double& _x,double& _y) const
          {
            CLLXYRad::GetXY(d2r(lat),d2r(lon),_x,_y);
          }

          void GetLL(double x,double y,double& _lat,double& _lon) const
          {
            double lat_r,lon_r;
            CLLXYRad::GetLL(x,y,lat_r,lon_r);
            _lat = r2d(lat_r);
            _lon = r2d(lon_r);
          }

  private:
          static double d2r(double d) { return d*(M_PI/180.0); }
          static double r2d(double r) { return r*(180.0/M_PI); }
};


double CLLXYRad::mdist(double s1,double d1,double s2,double d2)
{
  double t = sin(s1)*sin(s2)+cos(s1)*cos(s2)*cos(d2-d1);
  t = MIN(t,+1);
  t = MAX(t,-1);
  return EARTH_RADIUS_METERS*acos(t);
}


void CLLXYRad::GetXY(double lat,double lon,double& _x,double& _y) const
{
  _y = EARTH_RADIUS_METERS*(lat-m_lat0);

  double t = sqr(mdist(m_lat0,m_lon0,lat,lon))-sqr(_y);
  t = t < 0 ? 0 : t;
  double sign = lon < m_lon0 ? -1 : +1;
  _x = sign*sqrt(t);
}


void CLLXYRad::GetLL(double x,double y,double& _lat,double& _lon) const
{
  _lat = y/EARTH_RADIUS_METERS+m_lat0;

  double angL = sqrt(sqr(x)+sqr(y))/EARTH_RADIUS_METERS;
  double sign = x < 0 ? -1 : +1;

  double t = (cos(angL)-sin(m_lat0)*sin(_lat))/(cos(m_lat0)*cos(_lat));
  t = MIN(t,+1);
  t = MAX(t,-1);
  
  _lon = sign*acos(t)+m_lon0;
}


////////////////////////


// all x,y,step,... - in meters
// all times - in msec
class CDeviation
{
          typedef struct {
           double x,y;
           double ts_msec;
          } TSENSOR;
          
          std::vector<TSENSOR> m_sensors;

  public:
          CDeviation();
          ~CDeviation();

          void AddSensor(double x,double y,double ts_msec);

          void Analyze(double center_x,double center_y,double radius,double step,double barrier_msec,
                       double& _best_x,double& _best_y,double& _avg_ts,double& _best_dev_msec,double& _barrier_radius) const;

  private:
          static double sqr(double x) { return x*x; }
          static double dist(double x1,double y1,double x2,double y2) { return sqrt(sqr(x1-x2)+sqr(y1-y2)); }

};


CDeviation::CDeviation()
{
}


CDeviation::~CDeviation()
{
}


void CDeviation::AddSensor(double x,double y,double ts_msec)
{
  TSENSOR e;

  e.x = x;
  e.y = y;
  e.ts_msec = ts_msec;

  m_sensors.push_back(e);
}


void CDeviation::Analyze(double center_x,double center_y,double radius,double step,double barrier_msec,
                         double& _best_x,double& _best_y,double& _avg_ts,double& _best_dev_msec,double& _barrier_radius) const
{
  _best_x = center_x;
  _best_y = center_y;
  _avg_ts = 0;
  _best_dev_msec = +100000;
  _barrier_radius = +100000;

  const unsigned numsensors = m_sensors.size();

  if ( numsensors > 0 )
     {
       double start_x = center_x-radius;
       double start_y = center_y-radius;
       double end_x = center_x+radius;
       double end_y = center_y+radius;
       
       double barrier_min_x = end_x+1;
       double barrier_min_y = end_y+1;
       double barrier_max_x = start_x-1;
       double barrier_max_y = start_y-1;

       double deviation_min = +100000000;

       std::vector<double> t_ar;
       t_ar.resize(numsensors);

       for ( double y = start_y; y < end_y; y += step )
       for ( double x = start_x; x < end_x; x += step )
           {
             double t_avg = 0;
             for ( unsigned n = 0; n < numsensors; n++ )
                 {
                   t_ar[n] = m_sensors[n].ts_msec-dist(m_sensors[n].x,m_sensors[n].y,x,y)*(1000.0/SOUND_SPEED);
                   t_avg += t_ar[n];
                 }
             t_avg /= numsensors;

             double t_sigma = 0;
             for ( unsigned n = 0; n < numsensors; n++ )
                 {
                   t_sigma += sqr(t_ar[n]-t_avg);
                 }
             t_sigma /= numsensors;
             t_sigma = sqrt(t_sigma);

             if ( t_sigma < deviation_min )
                {
                  deviation_min = t_sigma;
                  _best_x = x;
                  _best_y = y;
                  _avg_ts = t_avg;
                  _best_dev_msec = t_sigma;
                }

             if ( t_sigma <= barrier_msec )
                {
                  barrier_min_x = MIN(barrier_min_x,x);
                  barrier_min_y = MIN(barrier_min_y,y);
                  barrier_max_x = MAX(barrier_max_x,x);
                  barrier_max_y = MAX(barrier_max_y,y);
                }
           }

       _barrier_radius = MAX(fabs(barrier_min_x-barrier_max_x),fabs(barrier_min_y-barrier_max_y))/2;
     }
}


/////////////////////////



// lat, lon in degrees!
class CSensorAnalyzer
{
          int m_processing_radius_meters;

          typedef struct {
           double lat,lon;
           double ts_msec;
          } TSOURCE;

          std::vector<TSOURCE> m_sns;

  public:
          CSensorAnalyzer(int processing_radius_meters);
          ~CSensorAnalyzer();

          void AddSensor(double lat,double lon,double ts_msec);
          BOOL Calculate(double& _lat,double& _lon,double& _ts_msec,double& _deviation_msec) const;
};


CSensorAnalyzer::CSensorAnalyzer(int processing_radius_meters)
  : m_processing_radius_meters(processing_radius_meters)
{
}


CSensorAnalyzer::~CSensorAnalyzer()
{
}


void CSensorAnalyzer::AddSensor(double lat,double lon,double ts_msec)
{
  TSOURCE e;

  e.lat = lat;
  e.lon = lon;
  e.ts_msec = ts_msec;

  m_sns.push_back(e);
}


BOOL CSensorAnalyzer::Calculate(double& _lat,double& _lon,double& _ts_msec,double& _deviation_msec) const
{
  BOOL rc = FALSE;

  if ( m_sns.size() > 2 )
     {
       // calc center point
       double lat_min = +1000;
       double lat_max = -1000;
       double lon_min = +1000;
       double lon_max = -1000;

       for ( unsigned n = 0; n < m_sns.size(); n++ )
           {
             lat_min = MIN(lat_min,m_sns[n].lat);
             lon_min = MIN(lon_min,m_sns[n].lon);
             lat_max = MAX(lat_max,m_sns[n].lat);
             lon_max = MAX(lon_max,m_sns[n].lon);
           }

       double lat_center = (lat_min + lat_max) / 2;
       double lon_center = (lon_min + lon_max) / 2;

       // convert to flat XY coords
       CLLXYDeg cnv(lat_center,lon_center);
       CDeviation dev;

       for ( unsigned n = 0; n < m_sns.size(); n++ )
           {
             double x,y;
             cnv.GetXY(m_sns[n].lat,m_sns[n].lon,x,y);
             dev.AddSensor(x,y,m_sns[n].ts_msec);
           }

       // step 1: inaccurate processing
       const double inaccurate_step = 10;
       const double accurate_step = 1;
       const double barrier_msec = 60;            // try another
       const double fatal_barrier_radius = 700;   // try another

       double best_x,best_y,avg_ts,best_dev_msec,barrier_radius;

       dev.Analyze(0,0,m_processing_radius_meters,inaccurate_step,barrier_msec,best_x,best_y,avg_ts,best_dev_msec,barrier_radius);

       if ( barrier_radius < fatal_barrier_radius )
          {
            // step 2: accurate processing
            dev.Analyze(best_x,best_y,inaccurate_step,accurate_step,0,best_x,best_y,avg_ts,best_dev_msec,barrier_radius);

            double lat,lon;
            cnv.GetLL(best_x,best_y,lat,lon);
            
            _lat = lat;
            _lon = lon;
            _ts_msec = avg_ts;
            _deviation_msec = best_dev_msec;

            rc = TRUE;
          }
     }

  return rc;
}


/////////////////////////



CHighLevelSectorsAnalyzer::CHighLevelSectorsAnalyzer()
{
}


CHighLevelSectorsAnalyzer::~CHighLevelSectorsAnalyzer()
{
}


void CHighLevelSectorsAnalyzer::PushSensorDetection(int sector,int device,int fdetect_ver,int fight_len_ms,float fight_db_amp,double lat,double lon,OURTIME ts)
{
  TSECTORVER sv;
  sv.sector = sector;
  sv.ver = fdetect_ver;

  TSENSORDATA sens;
  sens.len_ms = fight_len_ms;
  sens.db_amp = fight_db_amp;
  sens.lat = lat;
  sens.lon = lon;
  sens.ts = ts;

  TMap::iterator it = m_map.find(sv);
  if ( it == m_map.end() )
     {
       // first sensor in sector
       
       TSECTORDATA sd;
       sd.starttime = GetTickCount();
       sd.sensors[device] = sens;

       m_map[sv] = sd;
     }
  else
     {
       TSensorsMap& smap = it->second.sensors;

       if ( smap.find(device) == smap.end() )
          {
            smap[device] = sens;
          }
       else
          {
            ADD2LOG(("PushSensorDetection failed - already in queue, sector %d, device %d",sector,device));
          }
     }
}


BOOL CHighLevelSectorsAnalyzer::PopResult(int& _numsensors,int& _sector,int& _fdetect_ver,double& _lat,double& _lon,OURTIME& _ts)
{
  BOOL rc = FALSE;

  do {
  
   BOOL b_continue_loop = FALSE;

   for ( TMap::iterator it = m_map.begin(); it != m_map.end(); ++it )
       {
         const TSECTORDATA& sd = it->second;

         if ( GetTickCount() - sd.starttime > SECTOR_WAIT_TIME )
            {
              // process and then destroy this sector:
              
              const TSensorsMap& sensors = sd.sensors;  
              if ( sensors.size() > 2 )
                 {
                   // todo: check here if ts is very differs from average deviation of other ts's
                   //       also for db_amp, len_ms...

                   CSensorAnalyzer an(PROCESSING_RADIUS_METERS);

                   for ( TSensorsMap::const_iterator it2 = sensors.begin(); it2 != sensors.end(); ++it2 )
                       {
                         const TSENSORDATA& sd = it2->second;
                         an.AddSensor(sd.lat,sd.lon,(double)sd.ts);
                       }
                   
                   double lat,lon,ts_msec,deviation_msec;
                   if ( an.Calculate(lat,lon,ts_msec,deviation_msec) )
                      {
                        if ( deviation_msec > MAX_DEVIATION_MSEC )
                           {
                             ADD2LOG(("Deviation is too big %.0f, sector: %d, numsources: %d",deviation_msec,it->first.sector,sensors.size()));
                             b_continue_loop = TRUE;
                           }
                        else
                           {
                             _numsensors = sensors.size();
                             _sector = it->first.sector;
                             _fdetect_ver = it->first.ver;
                             _lat = lat;
                             _lon = lon;
                             _ts = (OURTIME)ts_msec;

                             ADD2LOG(("Calculation succedeed! sector: %d, numsources: %d, lat: %.6f, lon: %.6f, ts: %s UTC, dev: %.1f msec",it->first.sector,sensors.size(),lat,lon,OurTimeToString((OURTIME)ts_msec).c_str(),deviation_msec));

                             rc = TRUE;
                           }
                      }
                   else
                      {
                        ADD2LOG(("cannot Calculate() result, Sector: %d, numsources: %d",it->first.sector,sensors.size()));
                        b_continue_loop = TRUE;
                      }
                 }
              else
                 {
                   ADD2LOG(("cannot Calculate() result, Sector: %d, numsources: %d",it->first.sector,sensors.size()));
                   b_continue_loop = TRUE;
                 }

              m_map.erase(it);
              break;
            }
       }

   if ( !b_continue_loop )
      break;

  } while ( 1 );

  return rc;
}


