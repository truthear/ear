
#include "include.h"




CModem::CModem(EBoardUarts uart,int rate,unsigned recv_buff_size,int irq_priority)
{
  m_recv_buff_size = MAX(128,recv_buff_size);
  m_recv_buff_idx = 0;
  p_recv_buff = (unsigned char*)malloc(m_recv_buff_size);

  m_last_rx_time = CSysTicks::GetCounter();

  p_uart = new CBoardUART(uart,rate,true,true,OnRXCallbackWrapper,this,irq_priority);
}


CModem::~CModem()
{
  delete p_uart;
  free(p_recv_buff);
}


void CModem::SendATCmd(const char *cmd)
{
  if ( cmd )
     {
       while ( 1 )
       {
         char c = *cmd++;
         if ( c == 0 )
          break;

         p_uart->SendByte(c);
       }
     }
}


const unsigned char* CModem::RecvBuffAccess(unsigned& _wpos,unsigned& _size) const
{
  _wpos = m_recv_buff_idx;
  _size = m_recv_buff_size;
  return p_recv_buff;
}


void CModem::OnRXCallbackWrapper(void *parm,unsigned char data)
{
  reinterpret_cast<CModem*>(parm)->OnRXCallback(data);
}


void CModem::OnRXCallback(unsigned char data)
{
  unsigned idx = m_recv_buff_idx;

  p_recv_buff[idx] = data;

  idx++;
  if ( idx == m_recv_buff_size )
     {
       idx = 0;
     }

  m_recv_buff_idx = idx;  // update volatile idx at end

  m_last_rx_time = CSysTicks::GetCounter();
}



CTelitModem::CTelitModem(EBoardUarts uart,int rate,unsigned recv_buff_size,int irq_priority,
                         GPIO_TypeDef *reset_port,uint16_t reset_pin)
{
  p_reset_port = reset_port;
  m_reset_pin = reset_pin;

  // reset pin set first
  if ( p_reset_port )
     {
       GPIO_ResetBits(p_reset_port,m_reset_pin);
     }

  // here we should wait, because modem send some bytes of shit after power on   
  CSysTicks::Delay(300);

  // create modem object
  p_modem = new CModem(uart,rate,recv_buff_size,irq_priority);
}


CTelitModem::~CTelitModem()
{
  delete p_modem;

  if ( p_reset_port )
     {
       GPIO_SetBits(p_reset_port,m_reset_pin);
     }
}


void CTelitModem::ResetModem()
{
  if ( p_reset_port )
     {
       GPIO_SetBits(p_reset_port,m_reset_pin);
       CSysTicks::Delay(300);
       GPIO_ResetBits(p_reset_port,m_reset_pin);
       CSysTicks::Delay(2000);
     }
}


void CTelitModem::SendATCmd(const char *cmd)
{
  p_modem->SendATCmd(cmd);
}


const unsigned char* CTelitModem::RecvBuffAccess(unsigned& _wpos,unsigned& _size) const
{
  return p_modem->RecvBuffAccess(_wpos,_size);
}


unsigned CTelitModem::GetLastRXTime() const
{
  return p_modem->GetLastRXTime();
}




CBoardModem::CBoardModem(int rate,unsigned recv_buff_size,int irq_priority)
{
  // init reset pin first
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;  // PB0
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // create modem obj
  p_modem = new CTelitModem(BOARD_UART_GSM,rate,recv_buff_size,irq_priority,GPIOB,GPIO_InitStructure.GPIO_Pin);
}


CBoardModem::~CBoardModem()
{
  delete p_modem;
}


void CBoardModem::ResetModem()
{
  p_modem->ResetModem();
}


void CBoardModem::SendATCmd(const char *cmd)
{
  p_modem->SendATCmd(cmd);
}


const unsigned char* CBoardModem::RecvBuffAccess(unsigned& _wpos,unsigned& _size) const
{
  return p_modem->RecvBuffAccess(_wpos,_size);
}


unsigned CBoardModem::GetLastRXTime() const
{
  return p_modem->GetLastRXTime();
}


