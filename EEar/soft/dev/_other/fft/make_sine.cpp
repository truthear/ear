
#include <windows.h>
#include <math.h>
#include <stdio.h>

float f = 0.00000001;

void main()
{
  printf("%.10f\n",f);
 
  const int SAMPLE_RATE = 16000;
  
  FILE *f = fopen("!!!sine.raw","wb");

  const int LOOPS = 10;

  for ( int n = 0; n < SAMPLE_RATE*LOOPS; n++ )
      {
        short s = 30000*sin(2*3.141592653*1000*n/SAMPLE_RATE);
        fwrite(&s,2,1,f);
      }

  fclose(f);
}
