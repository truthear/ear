
#ifndef __ANALYSIS_H__
#define __ANALYSIS_H__


// this class uses ADD2LOG() !!!
class CHighLevelSectorsAnalyzer
{
          static const int PROCESSING_RADIUS_METERS = 8000;  // 16x16 km zone
          static const int MAX_DEVIATION_MSEC = 100;         // if "avg median deviation from sensors" more than this value, result will be ignored
          static const unsigned SECTOR_WAIT_TIME = 2000;    // wait time for sensors in sector


          struct TSECTORVER {
           int sector;
           int ver;

           bool operator < (const TSECTORVER& other) const
           {
             if ( sector < other.sector )
              return true;
             else
             if ( sector > other.sector )
              return false;
             else
              return ver < other.ver;
           }
          };

          struct TSENSORDATA {
           int len_ms;
           float db_amp;
           double lat;
           double lon;
           OURTIME ts;
          };

          typedef std::map<int,TSENSORDATA> TSensorsMap;  // [device_id] = TSENSORDATA
          
          struct TSECTORDATA {
           unsigned starttime;
           TSensorsMap sensors;  
          };

          struct CSectorVerCmp
          {
            bool operator()(const TSECTORVER& l,const TSECTORVER& r) const
            {
              return l<r;
            }
          };

          typedef std::map<TSECTORVER,TSECTORDATA,CSectorVerCmp> TMap;  // [sector,ver]=(start_time,[device]=(len,db,lat,lon,ts))

          TMap m_map;

  public:
          CHighLevelSectorsAnalyzer();
          ~CHighLevelSectorsAnalyzer();

          // ADD2LOG() already used inside, not needed to call manually after invoking these functions:
          void PushSensorDetection(int sector,int device,int fdetect_ver,int fight_len_ms,float fight_db_amp,double lat,double lon,OURTIME ts);
          BOOL PopResult(int& _numsensors,int& _sector,int& _fdetect_ver,double& _lat,double& _lon,OURTIME& _ts);

};



#endif
