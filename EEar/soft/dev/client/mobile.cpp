
#include "include.h"




CTelitMobile::CTelitMobile(int rate,unsigned max_queue_cmds)
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


CTelitMobile::~CTelitMobile()
{
  SAFEDELETE(p_trm);
  SAFEDELETE(p_modem);
}


void CTelitMobile::Poll()
{
  p_trm->Poll();
}


void CTelitMobile::ResetModem()
{
  p_trm->ResetModem();

  // not need to change sim/net status here...
}


bool CTelitMobile::Startup(bool use_auto_answer_mode)
{
  bool rc = true;
  
  rc = (p_trm->SyncProcessCmdSimple("AT") && rc);
  rc = (p_trm->SyncProcessCmdSimple(CFormat("AT+IPR=%d",m_baudrate)) && rc);
  rc = (p_trm->SyncProcessCmdSimple("AT+CMEE=2") && rc);
  rc = (p_trm->SyncProcessCmdSimple(use_auto_answer_mode?"ATS0=1":"ATS0=0") && rc);

  return rc;
}


void CTelitMobile::UpdateSIMStatus()
{
  p_trm->Push("AT+CPIN?",GeneralStatusCBWrapper,this,6000);
}


void CTelitMobile::UpdateNetStatus()
{
  p_trm->Push("AT+CREG?",GeneralStatusCBWrapper,this,2000);
}


void CTelitMobile::UpdateGPRSStatus()
{
  p_trm->Push("AT+CGREG?",GeneralStatusCBWrapper,this,2000);
}


void CTelitMobile::UpdateSignalQuality()
{
  p_trm->Push("AT+CSQ",GeneralStatusCBWrapper,this,2000);
}


void CTelitMobile::UpdateInternetConnectionStatus()
{
  p_trm->Push("AT#SGACT?",GeneralStatusCBWrapper,this,2000);
}


void CTelitMobile::GeneralStatusCBWrapper(void *parm,int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
{
  reinterpret_cast<CTelitMobile*>(parm)->GeneralStatusCB(id,cmd,answer,is_timeout,is_answered_ok);
}


void CTelitMobile::GeneralStatusCB(int id,const char *cmd,const char *answer,bool is_timeout,bool is_answered_ok)
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
            m_net_status = GetNetRegResultInternal(is_answered_ok,answer,"+CREG: 0,");
          }
       else
       if ( !strcmp(cmd,"AT+CGREG?") )
          {
            m_gprs_status = GetNetRegResultInternal(is_answered_ok,answer,"+CGREG: 0,");
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


ENetStatus CTelitMobile::GetNetRegResultInternal(bool is_answered_ok,const char *answer,const char *srch)
{
  ENetStatus rc = NET_UNKNOWN;
            
  if ( is_answered_ok )
     {
       const char *p = strstr(answer,srch);
       if ( p )
          {
            p += strlen(srch);
            
            if ( *p == '0' )
             rc = NET_NOT_REGISTERED;
            else
            if ( *p == '1' )
             rc = NET_REGISTERED_HOME;
            else
            if ( *p == '2' )
             rc = NET_SEARCHING;
            else
            if ( *p == '3' )
             rc = NET_DENIED;
            else
            if ( *p == '4' )
             rc = NET_UNKNOWN;
            else
            if ( *p == '5' )
             rc = NET_REGISTERED_ROAMING;
          }
     }

  return rc;
}


int CTelitMobile::InitUSSDRequest(const char *ussd,CTerminal::TCALLBACK cb,void *cbparm,unsigned max_time_to_wait_answer)
{
  return p_trm->Push(CFormat("AT+CUSD=1,\"%s\"",NNS(ussd)),cb,cbparm,max_time_to_wait_answer,max_time_to_wait_answer);
}


std::string CTelitMobile::DecodeUSSDAnswer(const char *answer)
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


int CTelitMobile::SendSMS(const char *phone,const char *text,CTerminal::TCALLBACK cb,void *cbparm,unsigned timeout)
{
  p_trm->Push("AT+CMGF=1");  // set text mode for SMS

  std::string s = "AT#CMGS=\"" + std::string(NNS(phone)) + "\",\"" + std::string(NNS(text)) + "\"";
  return p_trm->Push(s.c_str(),cb,cbparm,timeout);
}


void CTelitMobile::InitiateInternetConnection(const char *apn,const char *user,const char *pwd,unsigned timeout)
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


void CTelitMobile::ShutdownInternetConnection(unsigned timeout)
{
  p_trm->Push("AT#SGACT=1,0",NULL,NULL,timeout);
}


int CTelitMobile::SendStringTCP(const char *server,int port,const char *str,CTerminal::TCALLBACK cb,void *cbparm,
                                unsigned conn_timeout,unsigned total_timeout)
{
  conn_timeout /= 100;  // value in hundreds of msec
  conn_timeout = MAX(conn_timeout,10);
  conn_timeout = MIN(conn_timeout,1200);
  p_trm->Push(CFormat("AT#SCFG=1,1,0,0,%u,50",conn_timeout));

  p_trm->Push(CFormat("AT#IPCONSUMECFG=1,0,\"%s\",%d",NNS(server),port));

  std::string at = "AT#SSENDLINE=\""+std::string(NNS(str))+"\"";
  return p_trm->Push(at.c_str(),cb,cbparm,total_timeout);
}


int CTelitMobile::SendStringUDP(const char *server,int port,const char *str,CTerminal::TCALLBACK cb,void *cbparm,
                                unsigned total_timeout)
{
  p_trm->Push("AT#SCFG=1,1,0,0,50,50");

  p_trm->Push(CFormat("AT#IPCONSUMECFG=1,1,\"%s\",%d",NNS(server),port));

  std::string at = "AT#SSENDLINE=\""+std::string(NNS(str))+"\"";
  return p_trm->Push(at.c_str(),cb,cbparm,total_timeout);
}




