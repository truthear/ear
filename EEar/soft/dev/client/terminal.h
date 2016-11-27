
#ifndef __TERMINAL_H__
#define __TERMINAL_H__


class CBoardModem;


// this class uses CSysTicks, which should be initialized before!
// WARNING!!! class not IRQ safe!
class CTerminal
{
  public:
          typedef void (*TCALLBACK)(void*,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok);

  private:        
          unsigned m_creation_time;
          unsigned m_warming_up_time;
          bool b_modem_ready;

          unsigned m_min_at_cmd_interval;  // minimum time between complete one cmd and sending next

          typedef struct {
           int id;              // cmd unique id
           std::string cmd;     // AT command, with or without '\r' at end (depends of command)
           TCALLBACK cb;
           void *cbparm;
           unsigned min_wait;   // in ms, this time not includes time spending in queue
           unsigned max_wait;   // in ms, this time not includes time spending in queue
          } TCMD;

          typedef std::vector<TCMD*> TCMDSAR;
          TCMDSAR m_cmds;
          unsigned m_max_queue_cmds;

          int m_next_id;

          TCMD *p_work_cmd;
          unsigned m_work_cmd_starttime;
          unsigned m_wbuff_idx;  // idx in write buffer at moment just before AT-command send

          CBoardModem *p_modem;

  public:
          CTerminal(CBoardModem *_modem,unsigned max_queue_cmds,unsigned warming_up_time=1000,unsigned min_at_cmd_interval=25);
          ~CTerminal();

          int GetAvailQueueItemsCount() const { return (int)m_max_queue_cmds-(int)m_cmds.size(); }
          // returns -1 if queue is full, or unique id if operation success
          int Push(const char *atcmd,TCALLBACK cb=NULL,void *cbparm=NULL,unsigned max_wait=1000,unsigned min_wait=0,bool add_cr_at_end=true);
          void SyncProcessCmd(const char *atcmd,std::string& _answer,bool& _is_timeout,bool& _is_answered_ok,
                              unsigned command_max_wait=1000,unsigned command_min_wait=0,bool add_cr_at_end=true);
          bool SyncProcessCmdSimple(const char *atcmd,unsigned command_max_wait=1000,bool add_cr_at_end=true);
          
          void Poll();

          void ResetModem();

  private:
          static unsigned GetPrevBuffIdx(unsigned curr,unsigned buffsize) { return curr ? curr-1 : buffsize-1; }
          void DisposeWorkCmd(const char *answer,bool is_timeout,bool is_answered_ok);
          static void InternalSyncCB(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok);
          static void UnrollString(char *dst,const char *src,unsigned srcsize,unsigned start,unsigned end);

};



#endif

