
#include "include.h"


OURTIME Str2OurTime(const char *s)
{
  OURTIME t = 0;

  if ( lstrlen(s) == 12 )
     {
       char ss[16];
       lstrcpy(ss,s);
       ss[2] = ' ';
       ss[5] = ' ';
       ss[8] = ' ';

       int _h=0,_m=0,_s=0,_ms=0;
       
       sscanf(ss,"%d %d %d %d",&_h,&_m,&_s,&_ms);

       //char test[20];
       //sprintf(test,"%02d:%02d:%02d.%03d",_h,_m,_s,_ms);
       //if ( lstrcmp(test,s) )
       //   {
       //     printf("sscanf() error!\n");
       //   }

       SYSTEMTIME st;

       st.wYear = 2017;
       st.wMonth = 1;
       st.wDayOfWeek = 0;
       st.wDay = 1;
       st.wHour = _h;
       st.wMinute = _m;
       st.wSecond = _s;
       st.wMilliseconds = _ms;
       
       t = SystemTimeToOurTime(st);
     }

  return t;
}


void Push(CHighLevelSectorsAnalyzer& an,int dev_id,const char *s_time)
{
  static const struct { int dev_id; double lat,lon; } devs[] = 
  {
    { 1, 47.909653,37.787968 },
    { 2, 47.906002,37.783707 },
    { 3, 47.909035,37.786482 },
    { 4, 47.907422,37.783393 },
    { 5, 47.909685,37.785067 },
    { 666, 47.908404,37.795709 },  // fictive!!!
  };

  double lat=0,lon=0;

  for ( int n = 0; n < sizeof(devs)/sizeof(devs[0]); n++ )
      {
        if ( devs[n].dev_id == dev_id )
           {
             lat = devs[n].lat;
             lon = devs[n].lon;
             break;
           }

      }

  if ( lat+lon>0 )
     {
       an.PushSensorDetection(1,dev_id,100,100,20, lat,lon,Str2OurTime(s_time));
     }
  else
   printf("invalid device\n");
}


typedef std::vector<std::pair<int,const char*>> TExpData;


void Experiment(const TExpData& ar)
{
  CHighLevelSectorsAnalyzer an;

  for ( int n = 0; n < ar.size(); n++ )
      {
        Push(an,ar[n].first,ar[n].second);
      }

  int numsensors,sector,fdetect_ver;
  double lat,lon;
  OURTIME ts;
  
  while ( !an.PopResult(numsensors,sector,fdetect_ver,lat,lon,ts) )
  {
    Sleep(50);
  }

  printf("%.6f,%.6f, - %s\n",lat,lon,OurTimeToString(ts).c_str());
}



void main()
{
  { // experiment 1
    TExpData ar;
    ar.push_back(TExpData::value_type(1,"09:21:14.929"));  // 0
    ar.push_back(TExpData::value_type(2,"09:21:14.028"));  // на 20 позже
    ar.push_back(TExpData::value_type(3,"09:21:14.767"));  // на 28 позже
    ar.push_back(TExpData::value_type(4,"09:21:14.482"));  // на 32 позже
    //ar.push_back(TExpData::value_type(666,"09:21:14.990"));  //!!! fictive 975 m, 2.946 sec
    //ar.push_back(TExpData::value_type(5,"09:21:15.050"));  //!!! fictive 993 m, 3.0 sec +/-0.408
    Experiment(ar);
  }

  { // experiment 2
    TExpData ar;
    ar.push_back(TExpData::value_type(1,"09:30:47.236")); // 0
    ar.push_back(TExpData::value_type(2,"09:30:46.341")); // на 60 позже
    ar.push_back(TExpData::value_type(4,"09:30:46.820")); // на 90 позже
    ar.push_back(TExpData::value_type(5,"09:30:47.323")); // 0
    //ar.push_back(TExpData::value_type(3,"09:30:47.060"));  //!!! fictive 885 m, 2.673 sec -0.302
    //ar.push_back(TExpData::value_type(666,"09:30:47.390"));  //!!! fictive 994 m, 3.0 sec -0.302
    Experiment(ar);
  }

  { // experiment 3
    TExpData ar;
    ar.push_back(TExpData::value_type(1,"09:41:08.078"));   // 0
    ar.push_back(TExpData::value_type(2,"09:41:07.069"));   //позже на 70
    ar.push_back(TExpData::value_type(3,"09:41:07.842"));   // ранее на 28
    ar.push_back(TExpData::value_type(4,"09:41:07.555"));   // позже на 85
    ar.push_back(TExpData::value_type(5,"09:41:08.160"));   // позже на 50
    Experiment(ar);
  }

  { // experiment 4
    TExpData ar;
    ar.push_back(TExpData::value_type(1,"09:50:28.961"));   // 0
    ar.push_back(TExpData::value_type(2,"09:50:27.770"));   // позже на 30
    ar.push_back(TExpData::value_type(3,"09:50:28.699"));   // 0
    ar.push_back(TExpData::value_type(4,"09:50:28.246"));   // позже на 36
    ar.push_back(TExpData::value_type(5,"09:50:28.932"));   // позже на 10
    //ar.push_back(TExpData::value_type(666,"09:50:29.370"));  //!!! fictive 1100 m, 3.323 sec
    Experiment(ar);
  }

  { // experiment 5
    TExpData ar;
    ar.push_back(TExpData::value_type(1,"10:01:40.180"));    // позже на 20
    ar.push_back(TExpData::value_type(2,"10:01:39.007"));    // позже на 30
    ar.push_back(TExpData::value_type(3,"10:01:39.946"));    // позже на 26
    //ar.push_back(TExpData::value_type(4,"10:01:39.454"));  //!!! fictive 760 m, 2.300 sec
    //ar.push_back(TExpData::value_type(5,"10:01:40.154"));  //!!! fictive 993 m, 3.0 sec
    //ar.push_back(TExpData::value_type(666,"10:01:40.540"));  //!!! fictive 1120 m, 3.383 sec
    Experiment(ar);
  }
 
}


