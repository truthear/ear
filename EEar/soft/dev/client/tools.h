
#ifndef __TOOLS_H__
#define __TOOLS_H__


typedef long long OURTIME;   // time_t*1000+msec


OURTIME ConvertOurTime(int yyyy,int mm,int dd,int hh,int nn,int ss,int msec);
char* OurTimeToString(OURTIME t,char *s); // WARNING!!! not IRQhandler/thread safe! problem when simulatenous call in main and IRQ



#endif
