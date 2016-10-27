
#include "include.h"


const int SAMPLE_RATE = 16000;
const int MS1_SAMPLES = SAMPLE_RATE/1000;
const int MIN_FIGHT_LEN_MS = 90;
const int MAX_FIGHT_LEN_MS = 2000;
const float FIGHT_LEAP_DB = 10.0;


unsigned g_time = 0;



void main(int argc,char **argv)
{
  const char *filename = argc==2?argv[1]:"1.raw";
  
  FILE *f = fopen(filename,"rb");
  if ( f )
     {
       {
         CFDetector fd(SAMPLE_RATE,MIN_FIGHT_LEN_MS,MAX_FIGHT_LEN_MS,FIGHT_LEAP_DB);

         while (1)
         {
           short pcm[MS1_SAMPLES];

           if ( fread(pcm,sizeof(short),MS1_SAMPLES,f) != MS1_SAMPLES )
            break;

           fd.Push1ms(pcm);

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
}
