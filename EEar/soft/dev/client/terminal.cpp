
#include "include.h"




CTerminal::CTerminal(CBoardModem *_modem,unsigned max_queue_cmds,unsigned warming_up_time,unsigned min_at_cmd_interval)
{
  m_creation_time = CSysTicks::GetCounter();

  m_warming_up_time = warming_up_time;
  m_warming_up_time = MIN(m_warming_up_time,5000);
  m_warming_up_time = MAX(m_warming_up_time,1);

  b_modem_ready = false;

  m_min_at_cmd_interval = min_at_cmd_interval;

  m_max_queue_cmds = MAX(max_queue_cmds,1);
  m_cmds.reserve(m_max_queue_cmds);  // reallocations not needed

  m_next_id = 0;

  p_work_cmd = NULL;
  m_work_cmd_starttime = 0;
  m_wbuff_idx = 0;

  p_modem = _modem;
}


CTerminal::~CTerminal()
{
  SAFEDELETE(p_work_cmd);

  for ( unsigned n = 0; n < m_cmds.size(); n++ )
      {
        SAFEDELETE(m_cmds[n]);
      }
  m_cmds.clear();
}


int CTerminal::Push(const char *atcmd,TCALLBACK cb,void *cbparm,unsigned max_wait,unsigned min_wait,bool add_cr_at_end)
{
  int rc = -1;

  if ( IsStrEmpty(atcmd) )
     {
       atcmd = "AT";
     }

  if ( max_wait <= min_wait )
     {
       max_wait = min_wait+1;
     }

  if ( m_cmds.size() < m_max_queue_cmds )
     {
       TCMD *pcmd = new TCMD;

       pcmd->id = m_next_id;
       pcmd->cmd = atcmd;
       if ( add_cr_at_end )
          {
            pcmd->cmd += '\r';
          }
       pcmd->cb = cb;
       pcmd->cbparm = cbparm;
       pcmd->min_wait = min_wait;
       pcmd->max_wait = max_wait;

       m_cmds.push_back(pcmd);

       m_next_id++;
       if ( m_next_id < 0 )
          {
            m_next_id = 0;
          }

       rc = pcmd->id;
     }

  return rc;
}


typedef struct {
 bool complete;
 std::string answer;
 bool is_timeout;
 bool is_answered_ok;
} TINTERNALSYNCCB;


void CTerminal::InternalSyncCB(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  TINTERNALSYNCCB *i = (TINTERNALSYNCCB*)parm;

  i->complete = true;
  i->answer = NNS(answer);
  i->is_timeout = is_timeout;
  i->is_answered_ok = is_answered_ok;
}


void CTerminal::SyncProcessCmd(const char *atcmd,std::string& _answer,bool& _is_timeout,bool& _is_answered_ok,
                               unsigned command_max_wait,unsigned command_min_wait,bool add_cr_at_end)
{
  TINTERNALSYNCCB i;
  i.complete = false;

  while ( Push(atcmd,InternalSyncCB,&i,command_max_wait,command_min_wait,add_cr_at_end) < 0 )
  {
    Poll();
  }

  while ( !i.complete )
  {
    Poll();
  }

  _answer = i.answer;
  _is_timeout = i.is_timeout;
  _is_answered_ok = i.is_answered_ok;
}


bool CTerminal::SyncProcessCmdSimple(const char *atcmd,unsigned command_max_wait,bool add_cr_at_end)
{
  std::string answer;
  bool is_timeout = true;
  bool is_answered_ok = false;
  
  SyncProcessCmd(atcmd,answer,is_timeout,is_answered_ok,command_max_wait,0,add_cr_at_end);

  return !is_timeout && is_answered_ok;
}


void CTerminal::Poll()
{
  if ( !b_modem_ready )
     {
       // startup warming procedure

       if ( CSysTicks::GetCounter() - m_creation_time > m_warming_up_time )
          {
            b_modem_ready = true;  // complete!
          }
     }
  else
     {
       if ( p_work_cmd )
          {
            // check min time wait first
            if ( CSysTicks::GetCounter() - m_work_cmd_starttime >= p_work_cmd->min_wait )
               {
                 // get answer if ready
                 unsigned widx = 0;
                 unsigned buffsize = 0;
                 const char *buff = (const char*)p_modem->RecvBuffAccess(widx,buffsize);

                 if ( widx != m_wbuff_idx )  // something recv?
                    {
                      unsigned idx_prev1 = GetPrevBuffIdx(widx,buffsize);
                      unsigned idx_prev2 = GetPrevBuffIdx(idx_prev1,buffsize);
                      unsigned idx_prev3 = GetPrevBuffIdx(idx_prev2,buffsize);
                      
                      if ( buff[idx_prev1] == '\n' || (buff[idx_prev1] == ' ' && buff[idx_prev2] == '>' && buff[idx_prev3] == '\n') )
                         {
                           char *s = (char*)alloca(buffsize+1);  // no free needed
                           UnrollString(s,buff,buffsize,m_wbuff_idx,widx);

                           const std::string& atcmd = p_work_cmd->cmd;  // with or without \r at end!

                           const char *atstart = strstr(s,atcmd.c_str());
                           if ( !atstart )
                              {
                                DisposeWorkCmd("Buffer error",true/*inform as timeout*/,false);
                              }
                           else
                              {
                                const char *answer = atstart + atcmd.size();
                                bool is_answer_ok = (strstr(answer,"\r\nOK\r\n") != NULL || strstr(answer,"\r\n> ") != NULL);
                                bool is_answer_err = (strstr(answer,"\r\nERROR\r\n") != NULL || 
                                                      strstr(answer,"\r\n+CME ERROR:") != NULL || 
                                                      strstr(answer,"\r\n+CMS ERROR:") != NULL );
                                if ( is_answer_ok || is_answer_err )
                                   {
                                     DisposeWorkCmd(answer,false,is_answer_ok/*assume no is_answer_err*/);
                                   }
                              }
                         }
                    }

                 // check for timeout
                 if ( p_work_cmd )
                    {
                      if ( CSysTicks::GetCounter() - m_work_cmd_starttime > p_work_cmd->max_wait )
                         {
                           DisposeWorkCmd("Timeout",true,false);  // bad situation for AT-sequence
                         }
                    }
               }
          }
       
       if ( !p_work_cmd )
          {
            // ready to pop and add new cmd for processing?
            if ( !m_cmds.empty() ) 
               {
                 p_work_cmd = m_cmds[0];
                 m_cmds.erase(m_cmds.begin());

                 // optional delay
                 unsigned last_rx_time = p_modem->GetLastRXTime();
                 unsigned delta = CSysTicks::GetCounter() - last_rx_time;
                 if ( delta < m_min_at_cmd_interval )
                    {
                      CSysTicks::Delay(m_min_at_cmd_interval-delta);
                    }
                    
                 m_work_cmd_starttime = CSysTicks::GetCounter();
                 unsigned buffsize;
                 p_modem->RecvBuffAccess(m_wbuff_idx,buffsize);  // save buff position
                 p_modem->SendATCmd(p_work_cmd->cmd.c_str());
               }
          }
     }
}


void CTerminal::DisposeWorkCmd(const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( p_work_cmd )
     {
       TCMD *t = p_work_cmd;
       p_work_cmd = NULL;

       if ( t->cb )
          {
            std::string cmd = t->cmd;
            if ( !cmd.empty() && cmd[cmd.size()-1] == '\r' )
               {
                 cmd.resize(cmd.size()-1);  // remove \r at end
               }
            
            t->cb(t->cbparm,t->id,cmd.c_str(),answer,is_timeout,is_answered_ok);
          }

       delete t;
     }
}


void CTerminal::ResetModem()
{
  p_modem->ResetModem();

  DisposeWorkCmd("Modem reset performed",true,false);
}


void CTerminal::UnrollString(char *dst,const char *src,unsigned srcsize,unsigned start,unsigned end)
{
  if ( dst )
     {
       dst[0] = 0;

       if ( src && srcsize > 0 && start < srcsize && end < srcsize )
          {
            while ( start != end )
            {
              *dst++ = src[start++];
              start = (start == srcsize ? 0 : start);
            }

            *dst++ = 0; // term
          }
     }
}




