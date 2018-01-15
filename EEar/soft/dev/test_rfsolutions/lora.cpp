
#include "include.h"


// this class uses CSysTicks, which should be initialized before!
class CSemtechSX
{
  public:
          enum EChip {
           SX1272,  // currently only one is supported
          };

          enum ERadioFreq {
           RF_868MHz = 868000000,
           RF_915MHz = 915000000,
          };

          enum EBandWidth {
           BW_125KHz = 0,
           BW_250KHz = 1,
           BW_500KHz = 2,
          };

          enum ESpreadFactor {
           SF_7 = 7,
           SF_8 = 8,
           SF_9 = 9,
           SF_10 = 10,
           SF_11 = 11,
           SF_12 = 12,
          };

          enum ECodingRate {
           CR_4_5 = 1,
           CR_4_6 = 2,
           CR_4_7 = 3,
           CR_4_8 = 4,
          };

          typedef struct {
           int freq;
           EBandWidth bw;
           ESpreadFactor sf;
           ECodingRate cr;
           bool crc;
           bool iq_invert;
           int preamble_len;
          } TInit;

  private:
          enum EState {
           STATE_IDLE,
           STATE_SENDING,
           STATE_RECEIVER,
          };
          
          EChip m_chip;
          CPin::EPins m_reset_pin;
          volatile EState m_state;

  protected:
          CSemtechSX(EChip chip,CPin::EPins reset_pin);
          ~CSemtechSX();

          // reset and initialize chip, can be called many times as needed with different parms
          void Init(const TInit& i);

          // returns false if timeout expired, this should not be happens normally,
          // only SPI problem is one reason,
          // in this case Init() should be called after to reinitialize chip, also caller's SPI reinit is needed
          bool Send(const uint8_t *buff,uint8_t size,unsigned maxwait_ms);
          virtual void OnRecvPacket(const uint8_t *buff,uint8_t size,float rssi,float snr) = 0;
          virtual void OnRecvCrcError() = 0;

          virtual void SpiBegin() = 0;
          virtual uint8_t SpiInOut8(uint8_t out_data) = 0;
          virtual void SpiEnd() = 0;

          void OnDIO0();

  private:
          void Reset();

          void WriteBuffer(uint8_t addr,const uint8_t *buffer,uint8_t size);
          void ReadBuffer(uint8_t addr,uint8_t *buffer,uint8_t size);
          void WriteFifo(const uint8_t *buffer,uint8_t size);
          void ReadFifo(uint8_t *buffer,uint8_t size);
          void WriteReg(uint8_t addr,uint8_t data);
          uint8_t ReadReg(uint8_t addr);
          void WriteBits(uint8_t addr,uint8_t from_bit,uint8_t to_bit,uint8_t value);
          uint8_t ReadBits(uint8_t addr,uint8_t from_bit,uint8_t to_bit);
          void WriteBit(uint8_t addr,uint8_t bit,uint8_t value);
          uint8_t ReadBit(uint8_t addr,uint8_t bit);
};


class CLoraMote : public CSemtechSX
{
          typedef void (*TRECVCALLBACK)(bool crc_error,const void *data,unsigned size,float rssi,float snr); 

  private:        
          TInit m_init;
          bool b_initialized;
          CPin::EPins m_ant_rx_pin;
          CPin::EPins m_ant_tx_pin;
          SPI_TypeDef* m_SPIx;
          TRECVCALLBACK p_cb;
          void *p_cbparm;

  public:
          CLoraMote(EChip chip,CPin::EPins reset_pin,CPin::EPins ant_rx_pin,CPin::EPins ant_tx_pin,SPI_TypeDef* SPIx);
          ~CLoraMote();

          // reset and initialize chip, can be called many times as needed with different parms
          void Init(const TInit& i);

          // returns false if timeout expired, this should not be happens normally
          bool Send(const void *buff,unsigned size,unsigned maxwait_ms);
          void Recv(TRECVCALLBACK cb,void *cbparm);

  protected:
          void SpiBegin();
          uint8_t SpiInOut8(uint8_t out_data);
          void SpiEnd();


};



#define RADIO_MOSI                                  PA_7   //SPI1, do not change!
#define RADIO_MISO                                  PA_6   //SPI1, do not change!
#define RADIO_SCLK                                  PA_5   //SPI1, do not change!
#define RADIO_NSS                                   PA_4   //SPI1, do not change!
#define RADIO_RESET                                 PE_2
#define RADIO_ANT_SWITCH_RX                         PE_4
#define RADIO_ANT_SWITCH_TX                         PE_3
#define RADIO_DIO_0                                 PA_3














