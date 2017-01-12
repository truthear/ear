
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

