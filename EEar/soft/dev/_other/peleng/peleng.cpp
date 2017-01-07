
#include "include.h"


const float SOUND_SPEED = 331;  // sound speed, m/sec
const unsigned MAX_SENSORS = 100;


template<
          int radius,  // in scale units (will be pixels on GUI-map)
          int scale    // how much meters in 1 scale unit (pixel)
        >
class CPeleng
{
          float m_fx;  // fight x, in units
          float m_fy;  // fight y, in units
          float m_ft;  // fight time in msec, should be any value

          struct {
           float x;  // in units
           float y;  // in units
           float t;  // time in msec
          } m_sensors[MAX_SENSORS];
          unsigned m_numsensors;

          float m_zone[radius][radius];  // average squared deviation, msec
          float m_deviation_min;  // in zone array
          float m_deviation_max;  // in zone array

          CRBuff *p_rb;  // GUI data ([radius][radius])

          unsigned m_calc_time_spent;
          int best_x;  // point with min deviation
          int best_y;  // point with min deviation

  public:
          CPeleng()
          {
            m_numsensors = 0;
            m_deviation_min = 0;
            m_deviation_max = 0;
            m_calc_time_spent = 0;
            best_x = 0;
            best_y = 0;

            p_rb = new CRBuff(radius,radius,24,TRUE);
          }

          ~CPeleng()
          {
            delete p_rb;
          }

          void FillRandomData(unsigned rand_seed,
                              unsigned sensors_cnt,
                              int max_random_delta_ms  // max random deviation in msec, 1=strict
                              )
          {
            max_random_delta_ms = MAX(max_random_delta_ms,1);
            
            srand(rand_seed);

            m_numsensors = sensors_cnt;

            m_fx = GetRandomCoordinate();
            m_fy = GetRandomCoordinate();
            m_ft = 100.0;  // any value

            for ( unsigned n = 0; n < m_numsensors; n++ )
                {
                  m_sensors[n].x = GetRandomCoordinate();
                  m_sensors[n].y = GetRandomCoordinate();
                  m_sensors[n].t = m_ft+(dist(m_fx,m_fy,m_sensors[n].x,m_sensors[n].y)*scale/SOUND_SPEED)*1000+((rand()%max_random_delta_ms)-max_random_delta_ms/2);
                }
          }

          void Calculate()
          {
            unsigned t1 = GetTickCount();

            if ( m_numsensors > 0 )
               {
                 m_deviation_min = +100000000;
                 m_deviation_max = -100000000;

                 for ( int x = 0; x < radius; x++ )
                 for ( int y = 0; y < radius; y++ )
                     {
                       float t_ar[MAX_SENSORS];
                       
                       for ( unsigned n = 0; n < m_numsensors; n++ )
                           {
                             t_ar[n] = m_sensors[n].t-dist(m_sensors[n].x,m_sensors[n].y,x,y)*scale/SOUND_SPEED*1000;
                           }

                       float t_avg = 0;
                       for ( unsigned n = 0; n < m_numsensors; n++ )
                           {
                             t_avg += t_ar[n];
                           }
                       t_avg /= m_numsensors;

                       float t_sigma = 0;
                       for ( unsigned n = 0; n < m_numsensors; n++ )
                           {
                             t_sigma += sqr(t_ar[n]-t_avg);
                           }
                       t_sigma /= m_numsensors;
                       t_sigma = sqrt(t_sigma);

                       m_zone[x][y] = t_sigma;

                       m_deviation_max = MAX(m_deviation_max,t_sigma);

                       if ( t_sigma < m_deviation_min )
                          {
                            m_deviation_min = t_sigma;
                            best_x = x;
                            best_y = y;
                          }
                     }
               }

            unsigned t2 = GetTickCount();
            m_calc_time_spent = t2-t1;
          }

          // drawn calculated data on rbuff
          void Draw(float barrier_ms)
          {
            p_rb->Clear();
            
            int barrier_min_x = +10000;
            int barrier_min_y = +10000;
            int barrier_max_x = -10000;
            int barrier_max_y = -10000;

            for ( int x = 0; x < radius; x++ )
            for ( int y = 0; y < radius; y++ )
                {
                  float dev = m_zone[x][y];
                  //float level = (dev-m_deviation_min)/(m_deviation_max-m_deviation_min);
                  //int ilevel = 255-(int)(level*255.0);
                  //int color = (dev <= barrier_ms ? RGB(255,0,0) : RGB(ilevel,ilevel,ilevel));
                  //p_rb->SetPixel24(x,y,color);

                  if ( dev <= barrier_ms )
                     {
                       p_rb->SetPixel24(x,y,RGB(255,0,0));

                       barrier_min_x = MIN(barrier_min_x,x);
                       barrier_min_y = MIN(barrier_min_y,y);
                       barrier_max_x = MAX(barrier_max_x,x);
                       barrier_max_y = MAX(barrier_max_y,y);
                     }
                }

            for ( unsigned n = 0; n < m_numsensors; n++ )
                {
                  DrawPoint((int)m_sensors[n].x,(int)m_sensors[n].y,RGB(0,255,0));
                }

            DrawPoint((int)m_fx,(int)m_fy,RGB(255,0,255));
            DrawPoint(best_x,best_y,RGB(255,255,0));

            {
              int radius_m = dist(barrier_min_x,barrier_min_y,barrier_max_x,barrier_max_y)*scale/2;
              int error_m = dist(best_x,best_y,m_fx,m_fy)*scale;
              char s[200];
              sprintf(s,"error: %d m, radius: %d m, dev_fxfy: %.1f msec, dev_best: %.1f msec, time: %d msec",error_m,radius_m,m_zone[(int)m_fx][(int)m_fy],m_zone[best_x][best_y],m_calc_time_spent);
              
              RECT r = {0,0,1500,50};
              
              DrawText(p_rb->GetHDC(),s,-1,&r,DT_LEFT|DT_SINGLELINE);
            }

          }

          BOOL GetData(int x,int y,float& _deviation_abs_ms,float& _deviation_rel) const
          {
            BOOL rc = FALSE;

            if ( (x >= 0 && x < radius) && (y >= 0 && y < radius) )
               {
                 _deviation_abs_ms = m_zone[x][y];
                 _deviation_rel = (_deviation_abs_ms-m_deviation_min)/(m_deviation_max-m_deviation_min);
                 rc = TRUE;
               }

            return rc;
          }

          void OnPaint(HWND hwnd,HDC hdc,int cwidth,int cheight) const
          {
            p_rb->PaintTo(hdc,0,0);
          }

  private:
          static double sqr(double x) { return x*x; }
          static double dist(double x1,double y1,double x2,double y2) { return sqrt(sqr(x1-x2)+sqr(y1-y2)); }

          static float GetRandomCoordinate()
          {
            const int border = 5;
            return (float)(rand() % (radius-border*2)) + border + ((float)(rand() % scale) / (float)scale);
          }

          void DrawPoint(int x,int y,int color)
          {
            const int size = 5;
            
            HBRUSH brush = CreateSolidBrush(color);

            RECT r;
            r.left = x-size/2;
            r.top = y-size/2;
            r.right = r.left + size;
            r.bottom = r.top + size;
            
            FillRect(p_rb->GetHDC(),&r,brush);

            DeleteObject(brush);
          }
};


static const int RADIUS = 740;  // pixels (discrete units)
static const int SCALE = 10;    // RADIUS*SCALE=real meters

CPeleng<RADIUS,SCALE> plg;

unsigned g_seed = 1;
int g_numsensors = 5;
int g_max_deviation_delta_ms = 16;
float g_barrier_ms = 60;



void UpdateAll(HWND hwnd)
{
  plg.FillRandomData(g_seed,g_numsensors,g_max_deviation_delta_ms);
  plg.Calculate();
  plg.Draw(g_barrier_ms);

  if ( hwnd )
     {
       InvalidateRect(hwnd,NULL,FALSE);
       SetWindowText(hwnd,"");
     }
}


LRESULT CALLBACK WndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
  switch ( message )
  {
    case WM_MOUSEMOVE:
                  {
                    int x = (int)(short)LOWORD(lParam);
                    int y = (int)(short)HIWORD(lParam);

                    float dev_abs_ms,dev_rel;
                    if ( plg.GetData(x,y,dev_abs_ms,dev_rel) )
                       {
                         char s[100];
                         sprintf(s,"dev: %.1f msec",dev_abs_ms);
                         SetWindowText(hwnd,s);
                       }
                    else
                       {
                         SetWindowText(hwnd,"");
                       }

                    return 0;
                  }
    
    case WM_PAINT:
                  {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hwnd,&ps);
                    if ( hdc )
                       {
                         RECT r;
                         GetClientRect(hwnd,&r);
                         plg.OnPaint(hwnd,hdc,r.right-r.left,r.bottom-r.top);
                       }
                    EndPaint(hwnd,&ps);
                    return 0;
                  }
    
    case WM_KEYDOWN:
                     {
                       BOOL is_shift = (GetAsyncKeyState(VK_SHIFT)&0x8000) ? TRUE : FALSE;
                       BOOL is_ctrl = (GetAsyncKeyState(VK_CONTROL)&0x8000) ? TRUE : FALSE;
                       
                       if ( wParam == VK_ESCAPE )
                          {
                            PostQuitMessage(1);
                          }
                       else
                       if ( wParam == VK_RETURN )
                          {
                            g_seed = GetTickCount();
                            UpdateAll(hwnd);
                          }
                       else
                       if ( wParam == VK_ADD && !is_shift && !is_ctrl )
                          {
                            g_max_deviation_delta_ms++;
                            UpdateAll(hwnd);
                          }
                       else
                       if ( wParam == VK_SUBTRACT && !is_shift && !is_ctrl  )
                          {
                            g_max_deviation_delta_ms--;
                            if ( g_max_deviation_delta_ms < 0 )
                             g_max_deviation_delta_ms = 0;
                            UpdateAll(hwnd);
                          }

                       break;
                     }
    
    case WM_CLOSE:
                     PostQuitMessage(1);
                     return 0;
  }

  return DefWindowProc(hwnd,message,wParam,lParam);
}


int WINAPI WinMain(HINSTANCE hThisInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdParam,int nCmdShow)
{
  UpdateAll(NULL);
  
  HINSTANCE instance = GetModuleHandle(NULL);
  
  WNDCLASS wc;
  ZeroMemory(&wc,sizeof(wc));
  wc.hCursor = LoadCursor(NULL,IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpfnWndProc = WndProc;
  wc.hInstance = instance;
  wc.lpszClassName = MAKEINTATOM(234);
  RegisterClass(&wc);

  HWND hwnd = CreateWindowEx(0,wc.lpszClassName,NULL,WS_OVERLAPPED|WS_SYSMENU|WS_MAXIMIZEBOX|WS_MINIMIZEBOX,0,0,500,500,NULL,NULL,instance,NULL);

  ShowWindow(hwnd,SW_SHOWMAXIMIZED);
  UpdateWindow(hwnd);

  MSG msg;
  while ( GetMessage(&msg,NULL,0,0) )
  {
    DispatchMessage(&msg);        
  }
  
  DestroyWindow(hwnd);
  UnregisterClass(wc.lpszClassName,instance);
}


