
#ifndef __LOG_H__
#define __LOG_H__



// WARNING!!! Not thread/IRQ safe!
// WARNING!!! SDCard and fatfs must be initialized first!
// WARNING!!! If you need correct time in log CRTC::Init() must be called first
// WARNING!!! If you need stdut echo CDebugger::Init() must be called first
class CLog
{
          static const unsigned MAX_LOG_STR_SIZE = 1024;   // uses stack memory!

          char m_filename[64];
          bool b_stdout_echo;

  public:
          CLog(const char *filename,bool stdout_echo=false);
          ~CLog();

          void Add(const char *format,...);
};



#endif
