
#include "include.h"


#define M_PI 3.14159265358979323846

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
                       double& _best_x,double& _best_y,double& _best_dev_msec,double& _barrier_radius) const;

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
                         double& _best_x,double& _best_y,double& _best_dev_msec,double& _barrier_radius) const
{
  _best_x = center_x;
  _best_y = center_y;
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



















