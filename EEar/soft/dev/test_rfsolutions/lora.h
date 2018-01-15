
#ifndef __LORA_H__
#define __LORA_H__


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

          struct TRadio 
          {
            int freq;
            EBandWidth bw;
            ESpreadFactor sf;
            ECodingRate cr;
            bool crc;
            bool iq_invert;
            int preamble_len;
            uint8_t sync_word;
            TRadio() : freq(RF_868MHz), bw(BW_500KHz), sf(SF_7), cr(CR_4_5), crc(true), iq_invert(false), preamble_len(8), sync_word(0x12) {}
          };

  private:
          enum EState {
           STATE_UNINITIALIZED,
           STATE_IDLE,
           STATE_SENDING,
           STATE_RECEIVER,
          };
          
          EChip m_chip;
          struct {
           CPin::EPins reset;
           CPin::EPins sclk;
           CPin::EPins miso;
           CPin::EPins mosi;
           CPin::EPins nss;
           SPI_TypeDef* SPIx;
          } m_pins;
          volatile EState m_state;

  protected:
          CSemtechSX(EChip chip,CPin::EPins reset,CPin::EPins sclk,CPin::EPins miso,CPin::EPins mosi,CPin::EPins nss,SPI_TypeDef* SPIx);
          ~CSemtechSX();
          
          void Init(const TRadio& i); // reset and initialize chip and SPI, can be called many times as needed with different parms

          bool Send(const uint8_t *buff,uint8_t size); // do not wait until packet transmitted on air, returns false if not initialized or prev sending in progress
          bool IsSendingInProgress() const;
          int GetTimeOnAirMs(const TRadio& i,unsigned packet_size) const;

          bool StartReceiverMode(); // returns false if not initialized
          virtual void OnRecvPacket(const uint8_t *buff,uint8_t size,float rssi,float snr) = 0;
          virtual void OnRecvCrcError() = 0;

          void OnDIO0();

  private:
          void Reset();

          void SpiInit();
          uint8_t SpiInOut8(uint8_t out_data);
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
          void SetOpMode(uint8_t mode);
};


class CLoraMote : public CSemtechSX
{
  public:
          // if crc_error==TRUE, other args are NULLs
          typedef void (*TRECVCALLBACK)(bool crc_error,const void *data,unsigned size,float rssi,float snr); 

  private:        
          CPin::EPins m_dio0;
          CPin::EPins m_ant_rx;
          CPin::EPins m_ant_tx;
          TRadio m_radio;
          TRECVCALLBACK p_cb;
          void *p_cbparm;

  public:
          CLoraMote(EChip chip,CPin::EPins reset,CPin::EPins sclk,CPin::EPins miso,CPin::EPins mosi,CPin::EPins nss,SPI_TypeDef* SPIx
                    CPin::EPins dio0,CPin::EPins ant_rx,CPin::EPins ant_tx,const TRadio& radio);
          ~CLoraMote();

          // returns false if prev sending is in progress after timeout expired, 
          // if maxwait time specified correctly (according to GetTimeOnAirMs()) then only SPI problem can cause a problem,
          // in this case chip will be reinitialized and repeat of Send() can be performed
          // generally this return code can be used mainly for debug purposes only and should normally never happens
          // max size is 256
          bool Send(const void *buff,unsigned size,unsigned maxwait_ms);
          bool IsSendingInProgress() const;  // for debug only
          int GetTimeOnAirMs(unsigned packet_size) const;
          
          void StartReceiverMode(TRECVCALLBACK cb,void *cbparm);

  protected:
          void OnRecvPacket(const uint8_t *buff,uint8_t size,float rssi,float snr);
          void OnRecvCrcError();

  private:
          static void OnDIO0Wrapper(void *parm);
          void OnDIO0();
};


class CBoardLoraMote : public CLoraMote
{
  public:
          CBoardLoraMote(const TRadio& radio) : CLoraMote(SX1272,CPin::PE_2,CPin::PA_5,CPin::PA_6,CPin::PA_7,CPin::PA_4,SPI1,
                                                          CPin::PA_3,CPin::PE_4,CPin::PE_3,radio) {}
};


#endif
