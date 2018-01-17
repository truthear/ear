
#include "include.h"



// needed registers of SX-chip
enum {
 RegFifo = 0x00,
 RegOpMode = 0x01,
 RegFrfMsb = 0x06,
 RegFrfMid = 0x07,
 RegFrfLsb = 0x08,
 RegPaConfig = 0x09,
 RegOcp = 0x0B,
 RegLna = 0x0C,
 RegFifoAddrPtr = 0x0D,
 RegFifoTxBaseAddr = 0x0E,
 RegFifoRxBaseAddr = 0x0F,
 RegFifoRxCurrentAddr = 0x10,
 RegIrqFlagsMask = 0x11,
 RegIrqFlags = 0x12,
 RegRxNbBytes = 0x13,
 RegPktSnrValue = 0x19,
 RegPktRssiValue = 0x1A,
 RegModemConfig1 = 0x1D,
 RegModemConfig2 = 0x1E,
 RegPreambleMsb = 0x20,
 RegPreambleLsb = 0x21,
 RegPayloadLength = 0x22,
 RegFifoRxByteAddr = 0x25,
 RegDetectOptimize = 0x31,
 RegInvertIQ = 0x33,
 RegDetectionThreshold = 0x37,
 RegSyncWord = 0x39,
 RegDioMapping1 = 0x40,
 RegDioMapping2 = 0x41,
 RegPaDac = 0x5A,
};



CSemtechSX::CSemtechSX(EChip chip,CPin::EPins reset,CPin::EPins sclk,CPin::EPins miso,CPin::EPins mosi,CPin::EPins nss,SPI_TypeDef* SPIx)
{
  m_chip = chip;

  m_pins.reset = reset;
  m_pins.sclk = sclk;
  m_pins.miso = miso;
  m_pins.mosi = mosi;
  m_pins.nss = nss;
  m_pins.SPIx = SPIx;

  m_state = STATE_UNINITIALIZED;
}


CSemtechSX::~CSemtechSX()
{
  if ( m_state != STATE_UNINITIALIZED )
     {
       m_state = STATE_UNINITIALIZED;  // paranoja
       Reset();
     }
}


void CSemtechSX::Init(const TRadio& i)
{
  m_state = STATE_UNINITIALIZED;  // paranoja

  SpiInit();
  Reset();

  // now we in STANDBY mode, switch to SLEEP
  SetOpMode(OP_MODE_SLEEP);
  Sleep(2);
  // enable LORA (only in SLEEP mode!)
  WriteBit(RegOpMode,7,1);
  // return back to STANDBY mode
  SetOpMode(OP_MODE_STANDBY);
  Sleep(2);
  
  // at this point we in STANDBY mode with LORA modem selection
  // initialize LORA registers
  WriteReg(RegLna,0x23); // LNA Gain
  WriteReg(RegSyncWord,i.sync_word);
  WriteReg(RegPaConfig,0x8F);  // PA_BOOST
  WriteBits(RegPaDac,2,0,7);   // enable +20 dBm
  WriteBits(RegOcp,4,0,15);    // over current protection set to 120 mA

  uint32_t freq = (uint32_t)((double)i.freq/61.03515625);
  WriteReg(RegFrfMsb,(uint8_t)((freq >> 16) & 0xFF));
  WriteReg(RegFrfMid,(uint8_t)((freq >> 8) & 0xFF));
  WriteReg(RegFrfLsb,(uint8_t)((freq >> 0) & 0xFF));

  bool lowdropt = ((i.sf == SF_11 || i.sf == SF_12) && i.bw == BW_125KHz);
  WriteReg(RegModemConfig1,((uint8_t)i.bw<<6) | ((uint8_t)i.cr<<3) | (0 << 2) | (i.crc?2:0) | (lowdropt?1:0));
  WriteBits(RegModemConfig2,7,4,i.sf);
  WriteReg(RegPreambleMsb,(i.preamble_len>>8)&0xFF);
  WriteReg(RegPreambleLsb,i.preamble_len&0xFF);
  WriteBit(RegInvertIQ,6,i.iq_invert?1:0);

  m_state = STATE_IDLE;
}


bool CSemtechSX::Send(const uint8_t *buff,uint8_t size)
{
  bool rc = false;

  if ( m_state != STATE_UNINITIALIZED && m_state != STATE_SENDING )
     {
       m_state = STATE_IDLE;  // for IRQ
       SetOpMode(OP_MODE_STANDBY); // switch to standby mode

      if ( !buff || !size ) 
         {
           rc = true;
         }
      else
         {
           WriteReg(RegPayloadLength,size);
           WriteReg(RegFifoTxBaseAddr,0);
           WriteReg(RegFifoAddrPtr,0);
           WriteFifo(buff,size);
           WriteReg(RegIrqFlagsMask,0xF7);  // unmask TxDone
           WriteReg(RegDioMapping1,0x40);   // DIO0=TxDone

           m_state = STATE_SENDING;
           SetOpMode(OP_MODE_TRANSMIT);

           rc = true;
         }
     }

  return rc;
}


bool CSemtechSX::IsSendingInProgress() const
{
  return m_state == STATE_SENDING;
}


int CSemtechSX::GetTimeOnAirMs(const TRadio& i,unsigned packet_size) const
{
  double bw = 0.0;
  switch( i.bw )
  {
    case BW_125KHz:
        bw = 125e3;
        break;
    case BW_250KHz:
        bw = 250e3;
        break;
    case BW_500KHz:
        bw = 500e3;
        break;
  }

  bool lowdropt = ((i.sf == SF_11 || i.sf == SF_12) && i.bw == BW_125KHz);

  // Symbol rate : time for one symbol (secs)
  double rs = bw / ( 1 << i.sf );
  double ts = 1 / rs;
  // time of preamble
  double tPreamble = ( i.preamble_len + 4.25 ) * ts;
  // Symbol length of payload and time
  double tmp = ceil( ( 8 * packet_size - 4 * i.sf +
                       28 + 16 * (i.crc?1:0) -
                       ( false/*FixLen*/ ? 20 : 0 ) ) /
                       ( double )( 4 * ( i.sf -
                       ( lowdropt ? 2 : 0 ) ) ) ) *
                       ( i.cr + 4 );
  double nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );
  double tPayload = nPayload * ts;
  // Time on air
  double tOnAir = tPreamble + tPayload;
  // return ms secs
  int airTime = floor( tOnAir * 1e3 + 0.999 );

  return airTime;
}


bool CSemtechSX::StartReceiverMode()
{
  bool rc = false;

  if ( m_state != STATE_UNINITIALIZED )
     {
       if ( m_state == STATE_RECEIVER )
          {
            rc = true;
          }
       else
          {
            m_state = STATE_IDLE;  // for IRQ
            SetOpMode(OP_MODE_STANDBY); // switch to standby mode

            WriteReg(RegIrqFlagsMask,0x9F);  // unmask RxDone and PayloadCrcError
            WriteReg(RegDioMapping1,0x00);   // DIO0=RxDone
            WriteReg(RegFifoRxBaseAddr,0);
            WriteReg(RegFifoAddrPtr,0);

            m_state = STATE_RECEIVER;
            SetOpMode(OP_MODE_RECEIVER);

            rc = true;
          }
     }

  return rc;
}


// Warning!!! called from IRQ!
void CSemtechSX::OnDIO0()
{
  uint8_t irqs = ReadReg(RegIrqFlags);

  const uint8_t MASK_RXDONE = 0x40;
  const uint8_t MASK_CRCERR = 0x20;
  const uint8_t MASK_TXDONE = 0x08;

  uint8_t clear = 0;

  bool rx_done = false;
  bool crc_err = false;
  bool tx_done = false;

  if ( irqs & MASK_RXDONE )
     {
       rx_done = true;
       clear |= MASK_RXDONE;
     }

  if ( irqs & MASK_CRCERR )
     {
       crc_err = true;
       clear |= MASK_CRCERR;
     }

  if ( irqs & MASK_TXDONE )
     {
       tx_done = true;
       clear |= MASK_TXDONE;
     }

  WriteReg(RegIrqFlags,clear);

  if ( m_state == STATE_SENDING )
     {
       if ( tx_done )
          {
            m_state = STATE_IDLE;
          }
     }
  else
  if ( m_state == STATE_RECEIVER )
     {
       if ( crc_err )
          {
            OnRecvCrcError();
          }
       else
       if ( rx_done )
          {
            int PktSnr = (int)ReadReg(RegPktSnrValue);
            float snr = PktSnr*0.25;

            int PktRssi = (int)ReadReg(RegPktRssiValue);
            float rssi = -139.0 + PktRssi*1.0667;
            if ( snr < 0 )
               {
                 rssi += snr;
               }

            uint8_t size = ReadReg(RegRxNbBytes);

            uint8_t RxBuffer[256];
            memset(RxBuffer,0,sizeof(RxBuffer));
          
            if ( size )
               {
                 WriteReg(RegFifoAddrPtr,ReadReg(RegFifoRxCurrentAddr));
                 ReadFifo(RxBuffer,size);
               }

            OnRecvPacket(RxBuffer,size,rssi,snr);
          }
     }
}


void CSemtechSX::Reset()
{
  Sleep(20);
  CPin::InitAsInput(m_pins.reset);  // High-Z
  Sleep(11);
  CPin::InitAsOutput(m_pins.reset,1);
  Sleep(2);
  CPin::InitAsInput(m_pins.reset);  // High-Z
  Sleep(10);
}


void CSemtechSX::SpiInit()
{
  if ( m_pins.SPIx == SPI1 )
     {
       RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
     }
  else
  if ( m_pins.SPIx == SPI2 )
     {
       RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
     }
  else
  if ( m_pins.SPIx == SPI3 )
     {
       RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
     }

  SPI_Cmd(m_pins.SPIx, DISABLE);
  SPI_I2S_DeInit(m_pins.SPIx);
  
  CPin::InitAsAF(m_pins.mosi,5,GPIO_PuPd_DOWN);
  CPin::InitAsAF(m_pins.miso,5,GPIO_PuPd_DOWN);
  CPin::InitAsAF(m_pins.sclk,5,GPIO_PuPd_DOWN);
  CPin::InitAsOutput(m_pins.nss,1,GPIO_PuPd_UP);

  SPI_InitTypeDef i;
  i.SPI_Mode = SPI_Mode_Master;
  i.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  i.SPI_DataSize = SPI_DataSize_8b;
  i.SPI_CPOL = SPI_CPOL_Low;
  i.SPI_CPHA = SPI_CPHA_1Edge;
  i.SPI_FirstBit = SPI_FirstBit_MSB;
  i.SPI_NSS = SPI_NSS_Soft;
  i.SPI_CRCPolynomial = 7;

  { // set BaudRatePrescaler
    RCC_ClocksTypeDef RCC_Clocks;
    RCC_GetClocksFreq(&RCC_Clocks);
    uint32_t sysClkTmp = RCC_Clocks.HCLK_Frequency;
    uint32_t divisor = 0;

    while ( sysClkTmp > 10000000 )
    {
      divisor++;
      sysClkTmp /= 2;

      if ( divisor == 7 )
        break;
    }

    i.SPI_BaudRatePrescaler = ( ( ( divisor & 0x4 ) == 0 ) ? 0x0 : SPI_CR1_BR_2 ) |
                              ( ( ( divisor & 0x2 ) == 0 ) ? 0x0 : SPI_CR1_BR_1 ) |
                              ( ( ( divisor & 0x1 ) == 0 ) ? 0x0 : SPI_CR1_BR_0 );
  }

  SPI_Init(m_pins.SPIx,&i);
  SPI_Cmd(m_pins.SPIx,ENABLE);
}


uint8_t CSemtechSX::SpiInOut8(uint8_t out_data)
{
  while ( SPI_I2S_GetFlagStatus(m_pins.SPIx,SPI_I2S_FLAG_TXE) == RESET ) {}
  SPI_I2S_SendData(m_pins.SPIx,out_data);

  while ( SPI_I2S_GetFlagStatus(m_pins.SPIx,SPI_I2S_FLAG_RXNE) == RESET ) {}
  return (uint8_t)SPI_I2S_ReceiveData(m_pins.SPIx);
}


void CSemtechSX::WriteBuffer(uint8_t addr,const uint8_t *buffer,uint8_t size)
{
  CIRQDisable g;

  CPin::Reset(m_pins.nss);

  SpiInOut8(addr|0x80);

  for ( unsigned i = 0; i < size; i++ )
      {
        SpiInOut8(buffer[i]);
      }

  CPin::Set(m_pins.nss);
}


void CSemtechSX::ReadBuffer(uint8_t addr,uint8_t *buffer,uint8_t size)
{
  CIRQDisable g;

  CPin::Reset(m_pins.nss);

  SpiInOut8(addr&0x7F);

  for ( unsigned i = 0; i < size; i++ )
      {
        buffer[i] = SpiInOut8(0);
      }

  CPin::Set(m_pins.nss);
}


void CSemtechSX::WriteFifo(const uint8_t *buffer,uint8_t size)
{
  WriteBuffer(RegFifo,buffer,size);
}


void CSemtechSX::ReadFifo(uint8_t *buffer,uint8_t size)
{
  ReadBuffer(RegFifo,buffer,size);
}


void CSemtechSX::WriteReg(uint8_t addr,uint8_t data)
{
  WriteBuffer(addr,&data,1);
}


uint8_t CSemtechSX::ReadReg(uint8_t addr)
{
  uint8_t data = 0;
  ReadBuffer(addr,&data,1);
  return data;
}


void CSemtechSX::WriteBits(uint8_t addr,uint8_t from_bit,uint8_t to_bit,uint8_t value)
{
  uint8_t lo = MIN(from_bit,to_bit);
  uint8_t hi = MAX(from_bit,to_bit);
  uint8_t cnt = hi-lo+1;

  uint8_t mask = 0;

  for ( uint8_t n = 0; n < cnt; n++ )
      {
        mask <<= 1;
        mask |= 1;
      }

  mask <<= lo;
  mask = ~mask;

  uint8_t v = ReadReg(addr);

  WriteReg(addr,(v&mask)|(value<<lo));
}


uint8_t CSemtechSX::ReadBits(uint8_t addr,uint8_t from_bit,uint8_t to_bit)
{
  uint8_t lo = MIN(from_bit,to_bit);
  uint8_t hi = MAX(from_bit,to_bit);
  uint8_t cnt = hi-lo+1;

  uint8_t mask = 0;

  for ( uint8_t n = 0; n < cnt; n++ )
      {
        mask <<= 1;
        mask |= 1;
      }

  mask <<= lo;

  uint8_t v = ReadReg(addr);

  return (v&mask)>>lo;
}


void CSemtechSX::WriteBit(uint8_t addr,uint8_t bit,uint8_t value)
{
  WriteBits(addr,bit,bit,value);
}


uint8_t CSemtechSX::ReadBit(uint8_t addr,uint8_t bit)
{
  return ReadBits(addr,bit,bit);
}


void CSemtechSX::SetOpMode(uint8_t mode)
{
  WriteBits(RegOpMode,2,0,mode);
}


int CSemtechSX::GetOpMode()
{
  return ReadBits(RegOpMode,2,0);
}


///////////////////////



CLoraMote::CLoraMote(EChip chip,CPin::EPins reset,CPin::EPins sclk,CPin::EPins miso,CPin::EPins mosi,CPin::EPins nss,SPI_TypeDef* SPIx,
                     CPin::EPins dio0,CPin::EPins ant_rx,CPin::EPins ant_tx,const TRadio& radio)
  : CSemtechSX(chip,reset,sclk,miso,mosi,nss,SPIx)
{
  m_dio0 = dio0;
  m_ant_rx = ant_rx;
  m_ant_tx = ant_tx;
  m_radio = radio;
  p_cb = NULL;
  p_cbparm = NULL;

  CPin::InitAsInput(dio0,GPIO_PuPd_UP);
  CPin::SetInterrupt(dio0,OnDIO0Wrapper,this,EXTI_Trigger_Rising,0/*high priority*/);

  CPin::InitAsOutput(ant_rx,0,GPIO_PuPd_UP);
  CPin::InitAsOutput(ant_tx,0,GPIO_PuPd_UP);

  Init(radio);
}


CLoraMote::~CLoraMote()
{
  p_cb = NULL;
  
  CPin::Reset(m_ant_rx);
  CPin::Reset(m_ant_tx);
  //CPin::RemoveInterrupt(m_dio0);
}


bool CLoraMote::Send(const void *buff,unsigned size,unsigned maxwait_ms)
{
  bool rc = false;

  if ( !buff || !size )
     {
       rc = true;
     }
  else
     {
       size = MIN(size,255);

       if ( IsSendingInProgress() )
          {
            if ( maxwait_ms )
               {
                 unsigned starttime = GetTickCount();
                 while ( GetTickCount() - starttime < maxwait_ms && IsSendingInProgress() ) {}
               }
          }

       CPin::Reset(m_ant_rx);
       CPin::Set(m_ant_tx);

       rc = CSemtechSX::Send((const uint8_t*)buff,(uint8_t)size);

       if ( !rc )
          {
            p_cb = NULL;
            CPin::Reset(m_ant_rx);
            CPin::Reset(m_ant_tx);
            Init(m_radio);  //reinitialize chip
          }
     }

  return rc;
}


bool CLoraMote::IsSendingInProgress() const
{
  return CSemtechSX::IsSendingInProgress();
}


int CLoraMote::GetTimeOnAirMs(unsigned packet_size) const
{
  return CSemtechSX::GetTimeOnAirMs(m_radio,packet_size);
}


void CLoraMote::StartReceiverMode(TRECVCALLBACK cb,void *cbparm)
{
  p_cb = NULL;
  p_cbparm = cbparm;
  p_cb = cb;

  CPin::Reset(m_ant_tx);
  CPin::Set(m_ant_rx);

  CSemtechSX::StartReceiverMode();
}


// Warning! called from IRQ!
void CLoraMote::OnRecvPacket(const uint8_t *buff,uint8_t size,float rssi,float snr)
{
  if ( p_cb )
     {
       p_cb(p_cbparm,false,buff,size,rssi,snr);
     }
}


// Warning! called from IRQ!
void CLoraMote::OnRecvCrcError()
{
  if ( p_cb )
     {
       p_cb(p_cbparm,true,NULL,0,0,0);
     }
}


// Warning! called from IRQ!
void CLoraMote::OnDIO0Wrapper(void *parm)
{
  reinterpret_cast<CLoraMote*>(parm)->OnDIO0();
}


// Warning! called from IRQ!
void CLoraMote::OnDIO0()
{
  CSemtechSX::OnDIO0();
}


