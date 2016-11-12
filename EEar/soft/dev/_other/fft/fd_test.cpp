
#include "include.h"


const int SAMPLE_RATE = 16000;



void main(int argc,char **argv)
{
  const char *filename = argc==2?argv[1]:"1.raw";

  printf("%s:\n",filename);
  
  FILE *f = fopen(filename,"rb");
  if ( f )
     {
       unsigned g_time = 0;

       {
         CFDetector fd(SAMPLE_RATE,200);

         while (1)
         {
           const int MS1_SAMPLES = SAMPLE_RATE/1000;
           short pcm[MS1_SAMPLES];

           if ( fread(pcm,sizeof(short),MS1_SAMPLES,f) != MS1_SAMPLES )
            break;

           fd.Push1ms(g_time,pcm);

           unsigned fts;
           unsigned flength;
           float fdb;
           if ( fd.PopResult(fts,flength,fdb) )
              {
                printf("%.3f-%.3f (%d msec), %.1f dB\n",(float)fts/1000.0,(float)(fts+flength)/1000.0,flength,fdb);
              }

           g_time++;  // next msec
         }
       }
     
       fclose(f);
     }

  printf("\n");
}
