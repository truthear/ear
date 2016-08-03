
#ifndef __TOOLS_H__
#define __TOOLS_H__


#define MAX(a,b)   ((a)>(b)?(a):(b))
#define MIN(a,b)   ((a)<(b)?(a):(b))

#define CLEAROBJ(o)   memset(&(o),0,sizeof(o))


typedef signed long long OURTIME;   // time_t*1000+msec, must be signed (because of arithmetic operations)!


OURTIME ConvertOurTime(int yyyy,int mm,int dd,int hh,int nn,int ss,int msec);
char* OurTimeToString(OURTIME t,char *s); // WARNING!!! not IRQhandler/thread safe! problem when simulatenous call in main and IRQ



#endif
