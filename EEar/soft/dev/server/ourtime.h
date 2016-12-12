
#ifndef __OURTIME_H__
#define __OURTIME_H__


typedef __int64 OURTIME;   // number of msec since 2000-01-01 00:00:00


static const OURTIME OURTIME_1_DAY = 86400000;



void OurTimeToSystemTime(OURTIME d,SYSTEMTIME *st);
OURTIME SystemTimeToOurTime(const SYSTEMTIME *st);
OURTIME GetNowOurTime();
OURTIME GetNowOurTimeUTC0();
void OurTimeToString(char *s,OURTIME t);
std::string OurTimeToString(OURTIME t);



#endif

