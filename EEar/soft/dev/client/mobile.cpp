
#include "include.h"



CMobile::CMobile(int rate,unsigned max_queue_cmds)
{
  m_baudrate = rate;
  p_modem = new CBoardModem(rate,RECV_BUFF_SIZE);
  p_trm = new CTerminal(p_modem,max_queue_cmds);

  m_sim_status = SIM_ERROR;
  m_net_status = NET_UNKNOWN;
  m_gprs_status = NET_UNKNOWN;
  m_signal_quality = 0;
  b_internet_connected = false;
}


CMobile::~CMobile()
{
  SAFEDELETE(p_trm);
  SAFEDELETE(p_modem);
}


void CMobile::Poll()
{
  p_trm->Poll();
}


void CMobile::ResetModem()
{
  p_trm->ResetModem();

  // not need to change sim/net status here...
}


bool CMobile::Startup(bool use_auto_answer_mode)
{
  bool rc = true;
  
  rc = (rc && p_trm->PushAndWaitCompleteSimpleCmd(CFormat("AT+IPR=%d",m_baudrate)));
  rc = (rc && p_trm->PushAndWaitCompleteSimpleCmd("AT+CMEE=2"));
  rc = (rc && p_trm->PushAndWaitCompleteSimpleCmd(use_auto_answer_mode?"ATS0=1":"ATS0=0"));

  return rc;
}


void CMobile::UpdateSIMStatus()
{
  p_trm->Push("AT+CPIN?",GeneralStatusCBWrapper,this,6000);
}


void CMobile::UpdateNetStatus()
{
  p_trm->Push("AT+CREG?",GeneralStatusCBWrapper,this,2000);
}


void CMobile::UpdateGPRSStatus()
{
  p_trm->Push("AT+CGREG?",GeneralStatusCBWrapper,this,2000);
}


void CMobile::UpdateSignalQuality()
{
  p_trm->Push("AT+CSQ",GeneralStatusCBWrapper,this,2000);
}


void CMobile::UpdateInternetConnectionStatus()
{
  p_trm->Push("AT#SGACT?",GeneralStatusCBWrapper,this,2000);
}


void CMobile::GeneralStatusCBWrapper(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  reinterpret_cast<CMobile*>(parm)->GeneralStatusCB(id,cmd,answer,is_timeout,is_answered_ok);
}


void CMobile::GeneralStatusCB(int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  if ( !is_timeout && !IsStrEmpty(answer) )
     {
       if ( !strcmp(cmd,"AT+CPIN?") )
          {
            m_sim_status = is_answered_ok ? (strstr(answer,"+CPIN: READY")?SIM_READY:SIM_NOTREADY) : SIM_ERROR;
          }
       else
       if ( !strcmp(cmd,"AT+CREG?") )
          {
            if ( !is_answered_ok )
               {
                 m_net_status = NET_UNKNOWN;
               }
            else
               {
                 if ( strstr(answer,"+CREG: 0,0") )
                  m_net_status = NET_NOT_REGISTERED;
                 else
                 if ( strstr(answer,"+CREG: 0,1") )
                  m_net_status = NET_REGISTERED_HOME;
                 else
                 if ( strstr(answer,"+CREG: 0,2") )
                  m_net_status = NET_SEARCHING;
                 else
                 if ( strstr(answer,"+CREG: 0,3") )
                  m_net_status = NET_DENIED;
                 else
                 if ( strstr(answer,"+CREG: 0,4") )
                  m_net_status = NET_UNKNOWN;
                 else
                 if ( strstr(answer,"+CREG: 0,5") )
                  m_net_status = NET_REGISTERED_ROAMING;
                 else
                  m_net_status = NET_UNKNOWN;
               }
          }
       else
       if ( !strcmp(cmd,"AT+CGREG?") )
          {
            if ( !is_answered_ok )
               {
                 m_gprs_status = NET_UNKNOWN;
               }
            else
               {
                 if ( strstr(answer,"+CGREG: 0,0") )
                  m_gprs_status = NET_NOT_REGISTERED;
                 else
                 if ( strstr(answer,"+CGREG: 0,1") )
                  m_gprs_status = NET_REGISTERED_HOME;
                 else
                 if ( strstr(answer,"+CGREG: 0,2") )
                  m_gprs_status = NET_SEARCHING;
                 else
                 if ( strstr(answer,"+CGREG: 0,3") )
                  m_gprs_status = NET_DENIED;
                 else
                 if ( strstr(answer,"+CGREG: 0,4") )
                  m_gprs_status = NET_UNKNOWN;
                 else
                 if ( strstr(answer,"+CGREG: 0,5") )
                  m_gprs_status = NET_REGISTERED_ROAMING;
                 else
                  m_gprs_status = NET_UNKNOWN;
               }
          }
       else
       if ( !strcmp(cmd,"AT+CSQ") )
          {
            if ( !is_answered_ok )
               {
                 m_signal_quality = 0;
               }
            else
               {
                 const char *p = strstr(answer,"+CSQ: ");
                 if ( p )
                    {
                      p += 6;
                      const char *end = strstr(p,",");
                      if ( end )
                         {
                           unsigned digits = end-p;
                           if ( digits == 1 || digits == 2 )
                              {
                                char s[4];
                                CLEAROBJ(s);
                                s[0] = *p++;
                                if ( digits == 2 )
                                  s[1] = *p++;

                                m_signal_quality = atoi(s);
                              }
                         }
                    }
               }
          }
       else
       if ( !strcmp(cmd,"AT#SGACT?") )
          {
            b_internet_connected = is_answered_ok && strstr(answer,"#SGACT: 1,1");
          }
     }
}


int CMobile::InitUSSDRequest(const char *ussd,CTerminal::TCALLBACK cb,void *cbparm,unsigned max_time_to_wait_answer)
{
  return p_trm->Push(CFormat("AT+CUSD=1,\"%s\"",NNS(ussd)),cb,cbparm,max_time_to_wait_answer,max_time_to_wait_answer);
}


std::string CMobile::DecodeUSSDAnswer(const char *answer)
{
  std::string rc;

  if ( !IsStrEmpty(answer) )
     {
       const char *p = strstr(answer,"+CUSD: ");
       if ( p )
          {
            p = strchr(p,'\"');
            if ( p )
               {
                 const char *start = p+1;
                 const char *end = strstr(start,"\",");
                 if ( end )
                    {
                      while ( start != end )
                      {
                        char c = *start++;
                        if ( c == '\r' || c == '\n' )
                         c = ' ';
                        rc += c;
                      }
                    }
               }
          }
     }

  return rc;
}


int CMobile::SendSMS(const char *phone,const char *text,CTerminal::TCALLBACK cb,void *cbparm,unsigned timeout)
{
  p_trm->Push("AT+CMGF=1");

  std::string s = "AT#CMGS=\"" + std::string(NNS(phone)) + "\",\"" + std::string(NNS(text)) + "\"";
  return p_trm->Push(s.c_str(),cb,cbparm,timeout);
}


void CMobile::InitiateInternetConnection(const char *apn,const char *user,const char *pwd,unsigned timeout)
{
  p_trm->Push(CFormat("AT+CGDCONT=1,\"IP\",\"%s\"",NNS(apn)));
  p_trm->Push("AT#SCFG=1,1,300,90,600,50");  // set default socket parameters (needed)

  std::string s = "AT#SGACT=1,1";
  if ( !IsStrEmpty(user) )
     {
       s += ",\"";
       s += NNS(user);
       s += "\"";
       s += ",\"";
       s += NNS(pwd);
       s += "\"";
     }
  p_trm->Push(s.c_str(),NULL,NULL,timeout);  // error can be reported if context already activated!
}


void CMobile::ShutdownInternetConnection(unsigned timeout)
{
  p_trm->Push("AT#SGACT=1,0",NULL,NULL,timeout);
}


int CMobile::SendStringTCP(const char *server,int port,const char *str,CTerminal::TCALLBACK cb,void *cbparm,unsigned conn_timeout,unsigned total_timeout)
{
  conn_timeout /= 100;  // value in hundreds of msec
  conn_timeout = MAX(conn_timeout,10);
  conn_timeout = MIN(conn_timeout,1200);

  p_trm->Push(CFormat("AT#SCFG=1,1,0,0,%u,50",conn_timeout));
  p_trm->Push(CFormat("AT#IPCONSUMECFG=1,0,\"%s\",%d",NNS(server),port));

  std::string at = "AT#SSENDLINE=\""+std::string(NNS(str))+"\"";
  return p_trm->Push(at.c_str(),cb,cbparm,total_timeout);
}













