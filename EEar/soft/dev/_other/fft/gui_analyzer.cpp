
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <vector>
#include <complex>
#include "fft.h"



#define M_PI       3.14159265358979323846
#define MAX(a,b)   ((a)>(b)?(a):(b))
#define MIN(a,b)   ((a)<(b)?(a):(b))

typedef double fp_type;
typedef std::complex<fp_type> CPLX;


const unsigned lg_cnt = 7;
const unsigned ar_size = 1 << lg_cnt;
const int SAMPLE_RATE = 16000;
const unsigned spc_size = ar_size/2;
typedef fp_type SPECTRUM[spc_size];

CFFT<fp_type,lg_cnt> fft;


void MakeSpectrum(const short *pcm,fp_type *out)
{
  CPLX ar[ar_size];
  
  for ( unsigned n = 0; n < ar_size; n++ )
      {
        fp_type s = (fp_type)pcm[n]/32768.0;
        s *= 0.54 - 0.46 * cos(2*M_PI*n/(ar_size-1));   // hamming window
        ar[n] = CPLX(s);
      }

  fft.Perform(ar,false);

  for ( unsigned n = 0; n < spc_size; n++ )
      {
        out[n] = std::norm(ar[n]);  // faster than abs()
      }
}


FILE *f = NULL;
SPECTRUM prev, curr;
int iter = 0;
const char *filename = "";


BOOL ReadNext(HWND hwnd)
{
  short pcm[ar_size];

  if ( fread(pcm,sizeof(short),ar_size,f) != ar_size )
    return FALSE;
  
  MakeSpectrum(pcm,curr);

  char s[100];
  sprintf(s,"%s - %.3f sec",filename,(double)iter*ar_size/SAMPLE_RATE);
  SetWindowText(hwnd,s);

  iter++;

  return TRUE;
}


void PaintRect(HDC hdc,int x,int y,int w,int h,int color)
{
  HBRUSH brush = CreateSolidBrush(color);

  RECT r;
  r.left = x;
  r.top = y;
  r.right = r.left + w;
  r.bottom = r.top + h;
  
  FillRect(hdc,&r,brush);

  DeleteObject(brush);
}


fp_type dB(fp_type curr,fp_type base,fp_type min_value)
{
  if ( curr < min_value && base < min_value )
     {
       return 0;
     }
  else
     {
       curr = MAX(curr,min_value);
       base = MAX(base,min_value);

       return 20*log10(curr/base);
     }
}


double sqr(double x)
{
  return x*x;
}


void Paint(HWND hwnd,HDC hdc,int swidth,int sheight)
{
  const int MAXHEIGHT = sheight*2/3;
  const int MAXHEIGHT2 = MAXHEIGHT/2;
  PaintRect(hdc,0,sheight/2-MAXHEIGHT2,swidth,1,RGB(255,0,0));
  PaintRect(hdc,0,sheight/2+MAXHEIGHT2,swidth,1,RGB(255,0,0));
  PaintRect(hdc,0,sheight/2,swidth,1,RGB(255,0,0));

  int bw = swidth/spc_size;
  PaintRect(hdc,bw*spc_size,0,1,sheight,RGB(255,0,0));

  fp_type sum = 0;

  for ( int n = 0; n < spc_size; n++ )
      {
        fp_type delta = 0.5*dB(curr[n],prev[n],sqr(spc_size*0.0001));  // 0.5 because of we use norm(), but not abs()
        sum += delta;
        
        int v = delta/80.0*MAXHEIGHT2;  // +/- 80 dB is limit in our case
        if ( v >= 0 )
         PaintRect(hdc,n*bw,sheight/2-v,bw,v,RGB(255,255,255));
        else
         PaintRect(hdc,n*bw,sheight/2,bw,-v,RGB(255,255,255));
      }

  sum /= spc_size;

  char s[100];
  sprintf(s,"avg: %.1f dB",sum);
  
  RECT r;
  r.left = 10;
  r.right = swidth-r.left;
  r.top = 10;
  r.bottom = r.top+50;
  DrawText(hdc,s,-1,&r,DT_CENTER|DT_SINGLELINE|DT_VCENTER);
}



LRESULT CALLBACK WndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
  switch ( message )
  {
    case WM_PAINT:
                  {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hwnd,&ps);
                    if ( hdc )
                       {
                         RECT r;
                         GetClientRect(hwnd,&r);
                         Paint(hwnd,hdc,r.right-r.left,r.bottom-r.top);
                       }
                    EndPaint(hwnd,&ps);
                    return 0;
                  }
    
    case WM_KEYDOWN:
                     {
                       if ( wParam == VK_ESCAPE )
                          {
                            PostQuitMessage(1);
                          }
                       else
                       if ( wParam == VK_RETURN )
                          {
                            CopyMemory(prev,curr,sizeof(prev));
                            Beep(500,50);
                          }
                       else
                          {
                            if ( !ReadNext(hwnd) )
                              PostQuitMessage(1);
                            else
                              InvalidateRect(hwnd,NULL,TRUE);
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
  ZeroMemory(prev,sizeof(prev));
  ZeroMemory(curr,sizeof(curr));

  filename = __argc==2?__argv[1]:"1.raw";
  
  f = fopen(filename,"rb");
  if ( f )
     {
       HINSTANCE instance = GetModuleHandle(NULL);
       
       WNDCLASS wc;
       ZeroMemory(&wc,sizeof(wc));
       wc.hCursor = LoadCursor(NULL,IDC_ARROW);
       wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
       wc.lpfnWndProc = WndProc;
       wc.hInstance = instance;
       wc.lpszClassName = MAKEINTATOM(234);
       RegisterClass(&wc);

       int sw = GetSystemMetrics(SM_CXSCREEN);
       int sh = GetSystemMetrics(SM_CYSCREEN);
       
       HWND hwnd = CreateWindowEx(0,wc.lpszClassName,NULL,WS_OVERLAPPED|WS_SYSMENU|WS_MAXIMIZEBOX,0,0,sw,sh,NULL,NULL,instance,NULL);

       ReadNext(hwnd);

       ShowWindow(hwnd,SW_SHOWMAXIMIZED);
       UpdateWindow(hwnd);

       MSG msg;
       while ( GetMessage(&msg,NULL,0,0) )
       {
         DispatchMessage(&msg);        
       }
       
       DestroyWindow(hwnd);
       UnregisterClass(wc.lpszClassName,instance);
       
       fclose(f);
     }
}


