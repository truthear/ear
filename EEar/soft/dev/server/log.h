
#ifndef __LOG_H__
#define __LOG_H__


class CLog
{
          static const int RETROSPECTIVE_DAYS = 60;

  public:
          static void Add(const char *format,...);
          static BOOL SaveLog(const char *filename);

};


#define ADD2LOG(arglist)   { CLog::Add arglist; }



#endif
