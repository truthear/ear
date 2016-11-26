
#ifndef __MODEM_H__
#define __MODEM_H__


class CUART;


// this class uses CSysTicks, which should be initialized before!
class CModem
{
          unsigned char *p_recv_buff;  // circular recv buffer, should include AT-cmd length itself because of echo
          unsigned m_recv_buff_size;
          volatile unsigned m_recv_buff_idx;  // next write idx, updated from IRQ only
          volatile unsigned m_last_rx_time;   // last time of RX callback, updated from IRQ only

          CUART *p_uart;

  public:
          CModem(EBoardUarts uart,int rate,unsigned recv_buff_size,int irq_priority);
          ~CModem();

          void SendATCmd(const char *cmd);  // no \r added inside function!
          const unsigned char* RecvBuffAccess(unsigned& _wpos,unsigned& _size) const;
          unsigned GetLastRXTime() const { return m_last_rx_time; }

  private:
          static void OnRXCallbackWrapper(void*,unsigned char data);
          void OnRXCallback(unsigned char data);
};


// this class uses CSysTicks, which should be initialized before!
class CTelitModem
{
          GPIO_TypeDef *p_reset_port;
          uint16_t m_reset_pin;

          CModem *p_modem;

  public:
          // constructor takes about ~300 msec!
          CTelitModem(EBoardUarts uart,int rate,unsigned recv_buff_size,int irq_priority,
                      GPIO_TypeDef *reset_port,uint16_t reset_pin);
          ~CTelitModem();

          void ResetModem();  // takes about ~2.3 sec!

          void SendATCmd(const char *cmd);  // no \r added inside function!
          const unsigned char* RecvBuffAccess(unsigned& _wpos,unsigned& _size) const;
          unsigned GetLastRXTime() const;
};


// this class uses CSysTicks, which should be initialized before!
class CBoardModem
{
          CTelitModem *p_modem;

  public:
          // constructor takes about ~300 msec!
          CBoardModem(int rate,unsigned recv_buff_size,int irq_priority=4);
          ~CBoardModem();

          void ResetModem();  // takes about ~2.3 sec!

          void SendATCmd(const char *cmd);  // no \r added inside function!
          const unsigned char* RecvBuffAccess(unsigned& _wpos,unsigned& _size) const;
          unsigned GetLastRXTime() const;
};




#endif

