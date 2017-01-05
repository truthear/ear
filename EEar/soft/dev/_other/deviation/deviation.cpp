
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <vector>


#define MAX(a,b)   ((a)>(b)?(a):(b))
#define MIN(a,b)   ((a)<(b)?(a):(b))


#define SOUND_SPEED 331.0  // m/s



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


  const unsigned rnd_seed = 1;
  const int max_random_delta_ms = 20;
  const int numsensors = 7;
  const double zone_radius = 8000; // meters
  const double barrier_msec = 60;



double GetRandomCoordinate()
{
  return (rand() % (int)(zone_radius*2)) - zone_radius;
}

static double sqr(double x) { return x*x; }
static double dist(double x1,double y1,double x2,double y2) { return sqrt(sqr(x1-x2)+sqr(y1-y2)); }




void main()
{
  srand(rnd_seed);

  for ( int iter = 0; iter < 10; iter++ )
      {
        double fx = GetRandomCoordinate();
        double fy = GetRandomCoordinate();
        double fts = 100.0;  // any value

        CDeviation dev;

        for ( unsigned n = 0; n < numsensors; n++ )
            {
              double x = GetRandomCoordinate();
              double y = GetRandomCoordinate();
              double ts = fts+(dist(fx,fy,x,y)/SOUND_SPEED)*1000+((rand()%max_random_delta_ms)-max_random_delta_ms/2);
              dev.AddSensor(x,y,ts);
            }

        double best_x,best_y,best_dev,barrier_radius;
        
        const double inaccurate_step = 10;
        const double accurate_step = 1;
        
        dev.Analyze(0.0,0.0,zone_radius,inaccurate_step,barrier_msec,best_x,best_y,best_dev,barrier_radius);

        printf("[*] %5d x %5d, error: %.1f m,\t dev: %.1f msec, barrier: %.0f meters\n",(int)best_x,(int)best_y,dist(fx,fy,best_x,best_y),best_dev,barrier_radius);

        dev.Analyze(best_x,best_y,inaccurate_step,accurate_step,0,best_x,best_y,best_dev,barrier_radius);
        printf("    %5d x %5d, error: %.1f m,\t dev: %.1f msec\n",(int)best_x,(int)best_y,dist(fx,fy,best_x,best_y),best_dev);
      }
}



