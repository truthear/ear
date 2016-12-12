
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdio.h>
//#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <complex>
#include "dbg_uart.h"
#include "ticks.h"
#include "fft.h"


#define M_PI       3.14159265358979323846


typedef float fp_type;
typedef std::complex<fp_type> CPLX;

const unsigned lg_cnt = 7;
const unsigned ar_size = 1 << lg_cnt;

//  float l[ar_size/2];
//  float ll=1.45;


void Test()
{

  CFFT<fp_type,lg_cnt> fft;

  CPLX ar[ar_size];
  CPLX ar_orig[ar_size];
  fp_type norm[ar_size/2];

  for ( unsigned n = 0; n < ar_size; n++ )
      {
        ar[n] = ar_orig[n] = CPLX(sin(1.0*2*M_PI*n/ar_size)+0.5*sin(63.0*2*M_PI*n/ar_size));
      }

  unsigned t1 = CCPUTicks::GetCounter();

  fft.Perform(ar,false);

  unsigned t2 = CCPUTicks::GetCounter();

  for ( unsigned n = 0; n < ar_size/2; n++ )
      {
        norm[n] = std::abs(ar[n]);  // std::norm
      }

  for ( unsigned n = 0; n < ar_size/2; n++ )
      {
        printf("%.10f %.10f\n",norm[n],std::arg(ar[n]));
      }

  fft.Perform(ar,true);

  printf("\n");

  for ( unsigned n = 0; n < ar_size; n++ )
      {
        printf("%.10f %.10f\n",ar[n].real()-ar_orig[n].real(),ar[n].imag());
      }

  printf("\nTime: %d ticks\n",t2-t1);
}



int main(void)
{
  InitUART3();
  CCPUTicks::Init();

  Test();

  while(1){}
}

