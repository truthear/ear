
#ifndef __LOG_H__
#define __LOG_H__



// WARNING!!! Not thread/IRQ safe!
// WARNING!!! SDCard and fatfs must be initialized first if you need file output!
// WARNING!!! CRTC::Init() must be called first
// WARNING!!! If you need stdout echo - CDebugger::Init() must be called first
class CLog
{
          static const unsigned MAX_LOG_STR_SIZE = 1024;   // uses stack memory!

          char m_filename[64];
          bool b_stdout_echo;

  public:
          CLog(const char *filename=NULL,bool stdout_echo=false);
          ~CLog();

          void Add(const char *format,...);
};


#define ADD2LOG(arglist)   { p_log->Add arglist; }



#endif
