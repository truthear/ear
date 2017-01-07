
#ifndef __ANALYSIS_H__
#define __ANALYSIS_H__



// lat, lon in degrees!
class CSensorAnalyzer
{
          static const int PROCESSING_RADIUS_METERS = 8000;  // 16x16 km zone

          typedef struct {
           double lat,lon;
           double ts_msec;
          } TSOURCE;

          std::vector<TSOURCE> m_sns;

  public:
          CSensorAnalyzer();
          ~CSensorAnalyzer();

          void AddSensor(double lat,double lon,double ts_msec);
          BOOL Calculate(double& _lat,double& _lon,double& _ts_msec,double& _deviation_msec) const;
};



#endif
