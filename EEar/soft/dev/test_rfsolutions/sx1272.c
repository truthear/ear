/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Generic SX1272 driver implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include <math.h>
#include <string.h>
#include "board.h"
#include "radio.h"
#include "sx1272Regs-fsk.h"
#include "sx1272Regs-LoRa.h"


#define FREQ_STEP    61.03515625
#define RADIO_WAKEUP_TIME                           1 // [ms]  Radio wake-up time from sleep
#define LORA_MAC_PRIVATE_SYNCWORD                   0x12
#define LORA_MAC_PUBLIC_SYNCWORD                    0x34


/*!
 * Radio LoRa modem parameters
 */
typedef struct
{
    uint32_t Bandwidth;
    uint32_t Datarate;
    bool     LowDatarateOptimize;
    uint8_t  Coderate;
    uint16_t PreambleLen;
    bool     CrcOn;
    bool     IqInverted;
}RadioLoRaSettings_t;

/*!
 * Radio Settings
 */
typedef struct
{
    RadioState_t             State;
    RadioLoRaSettings_t      LoRa;
}RadioSettings_t;

/*!
 * Radio hardware and global parameters
 */
typedef struct SX1272_s
{
    Gpio_t        Reset;
    Gpio_t        DIO0;
    Spi_t         Spi;
    Gpio_t        AntRx;
    Gpio_t        AntTx;
    RadioSettings_t Settings;
}SX1272_t;


static SX1272_t SX1272;

void SX1272Reset( void );
void SX1272SetModem( RadioModems_t modem );
void SX1272Write( uint8_t addr, uint8_t data );
uint8_t SX1272Read( uint8_t addr );
void SX1272WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size );
void SX1272ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size );
void SX1272WriteFifo( uint8_t *buffer, uint8_t size );
void SX1272ReadFifo( uint8_t *buffer, uint8_t size );
void SX1272SetOpMode( uint8_t opMode );
void SX1272OnDio0Irq( void );



void SX1272Init()
{
    static const 
    struct
    {
        RadioModems_t Modem;
        uint8_t       Addr;
        uint8_t       Value;
    }
     RadioRegsInit[] = 
    {                                                 
        { MODEM_FSK , REG_LNA                , 0x23 },
        { MODEM_FSK , REG_RXCONFIG           , 0x1E },
        { MODEM_FSK , REG_RSSICONFIG         , 0xD2 },
        { MODEM_FSK , REG_AFCFEI             , 0x01 },
        { MODEM_FSK , REG_PREAMBLEDETECT     , 0xAA },
        { MODEM_FSK , REG_OSC                , 0x07 },
        { MODEM_FSK , REG_SYNCCONFIG         , 0x12 },
        { MODEM_FSK , REG_SYNCVALUE1         , 0xC1 },
        { MODEM_FSK , REG_SYNCVALUE2         , 0x94 },
        { MODEM_FSK , REG_SYNCVALUE3         , 0xC1 },
        { MODEM_FSK , REG_PACKETCONFIG1      , 0xD8 },
        { MODEM_FSK , REG_FIFOTHRESH         , 0x8F },
        { MODEM_FSK , REG_IMAGECAL           , 0x02 },
        { MODEM_FSK , REG_DIOMAPPING1        , 0x00 },
        { MODEM_FSK , REG_DIOMAPPING2        , 0x30 },
        { MODEM_LORA, REG_LR_DETECTOPTIMIZE  , 0x43 },
        { MODEM_LORA, REG_LR_PAYLOADMAXLENGTH, 0xFF },
        { MODEM_LORA, REG_LR_SYNCWORD        , LORA_MAC_PRIVATE_SYNCWORD },
    };

    uint8_t i;

    SpiInit( &SX1272.Spi, RADIO_MOSI, RADIO_MISO, RADIO_SCLK, RADIO_NSS );

    GpioInit( &SX1272.AntTx, RADIO_ANT_SWITCH_TX, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
    GpioInit( &SX1272.AntRx, RADIO_ANT_SWITCH_RX, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );

    GpioInit( &SX1272.DIO0, RADIO_DIO_0, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );

    SX1272Reset( );

    SX1272SetOpMode( RF_OPMODE_SLEEP );

    {
      EXTI_InitTypeDef EXTI_InitStructure;
      NVIC_InitTypeDef NVIC_InitStructure;
      
      // enable sysclk
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,ENABLE);
      
      // Connect Button EXTI Line to GPIO Pin
      SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA,EXTI_PinSource3);

      // Configure Button EXTI line
      EXTI_InitStructure.EXTI_Line = EXTI_Line3;
      EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
      EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
      EXTI_InitStructure.EXTI_LineCmd = ENABLE;
      EXTI_Init(&EXTI_InitStructure);

      // Enable and set Button EXTI Interrupt to the lowest priority
      NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure); 
    }

    for( i = 0; i < sizeof( RadioRegsInit ) / sizeof( RadioRegsInit[0] ); i++ )
    {
        SX1272SetModem( RadioRegsInit[i].Modem );
        SX1272Write( RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
    }

    SX1272.Settings.State = RF_IDLE;
}

void SX1272SetChannel( uint32_t freq )
{
    SX1272SetModem( MODEM_FSK );
    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
    SX1272Write( REG_FRFMSB, ( uint8_t )( ( freq >> 16 ) & 0xFF ) );
    SX1272Write( REG_FRFMID, ( uint8_t )( ( freq >> 8 ) & 0xFF ) );
    SX1272Write( REG_FRFLSB, ( uint8_t )( freq & 0xFF ) );
}

void SX1272SetTxConfig( 
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool crcOn, bool iqInverted )
{
    SX1272SetModem( MODEM_LORA );

    { // set max output TX power
      uint8_t paConfig = SX1272Read( REG_PACONFIG );
      paConfig = ( paConfig & RF_PACONFIG_PASELECT_MASK ) | RF_PACONFIG_PASELECT_PABOOST; 
      paConfig = ( paConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | 0x0F;
      SX1272Write( REG_PACONFIG, paConfig );

      uint8_t paDac = SX1272Read( REG_PADAC );
      paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_ON;
      SX1272Write( REG_PADAC, paDac );
    }

            SX1272.Settings.LoRa.Bandwidth = bandwidth;
            SX1272.Settings.LoRa.Datarate = datarate;
            SX1272.Settings.LoRa.Coderate = coderate;
            SX1272.Settings.LoRa.PreambleLen = preambleLen;
            SX1272.Settings.LoRa.CrcOn = crcOn;
            SX1272.Settings.LoRa.IqInverted = iqInverted;

            if( datarate > 12 )
            {
                datarate = 12;
            }
            else if( datarate < 6 )
            {
                datarate = 6;
            }
            if( ( ( bandwidth == 0 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
                ( ( bandwidth == 1 ) && ( datarate == 12 ) ) )
            {
                SX1272.Settings.LoRa.LowDatarateOptimize = 0x01;
            }
            else
            {
                SX1272.Settings.LoRa.LowDatarateOptimize = 0x00;
            }

            SX1272Write( REG_LR_MODEMCONFIG1,
                         ( SX1272Read( REG_LR_MODEMCONFIG1 ) &
                           RFLR_MODEMCONFIG1_BW_MASK &
                           RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                           RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK &
                           RFLR_MODEMCONFIG1_RXPAYLOADCRC_MASK &
                           RFLR_MODEMCONFIG1_LOWDATARATEOPTIMIZE_MASK ) |
                           ( bandwidth << 6 ) | ( coderate << 3 ) |
                           ( 0/*fixLen*/ << 2 ) | ( crcOn << 1 ) |
                           SX1272.Settings.LoRa.LowDatarateOptimize );

            SX1272Write( REG_LR_MODEMCONFIG2,
                        ( SX1272Read( REG_LR_MODEMCONFIG2 ) &
                          RFLR_MODEMCONFIG2_SF_MASK ) |
                          ( datarate << 4 ) );


            SX1272Write( REG_LR_PREAMBLEMSB, ( preambleLen >> 8 ) & 0x00FF );
            SX1272Write( REG_LR_PREAMBLELSB, preambleLen & 0xFF );

            if( datarate == 6 )
            {
                SX1272Write( REG_LR_DETECTOPTIMIZE,
                             ( SX1272Read( REG_LR_DETECTOPTIMIZE ) &
                               RFLR_DETECTIONOPTIMIZE_MASK ) |
                               RFLR_DETECTIONOPTIMIZE_SF6 );
                SX1272Write( REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF6 );
            }
            else
            {
                SX1272Write( REG_LR_DETECTOPTIMIZE,
                             ( SX1272Read( REG_LR_DETECTOPTIMIZE ) &
                             RFLR_DETECTIONOPTIMIZE_MASK ) |
                             RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
                SX1272Write( REG_LR_DETECTIONTHRESHOLD,
                             RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
            }
}

void SX1272SetRxConfig(  uint32_t bandwidth,
                         uint32_t datarate, uint8_t coderate,
                         uint16_t preambleLen,
                         uint16_t symbTimeout, 
                         bool crcOn, 
                         bool iqInverted )
{
    SX1272SetModem( MODEM_LORA );

    SX1272.Settings.LoRa.Bandwidth = bandwidth;
    SX1272.Settings.LoRa.Datarate = datarate;
    SX1272.Settings.LoRa.Coderate = coderate;
    SX1272.Settings.LoRa.PreambleLen = preambleLen;
    SX1272.Settings.LoRa.CrcOn = crcOn;
    SX1272.Settings.LoRa.IqInverted = iqInverted;

    if( datarate > 12 )
    {
        datarate = 12;
    }
    else if( datarate < 6 )
    {
        datarate = 6;
    }

    if( ( ( bandwidth == 0 ) && ( ( datarate == 11 ) || ( datarate == 12 ) ) ) ||
        ( ( bandwidth == 1 ) && ( datarate == 12 ) ) )
    {
        SX1272.Settings.LoRa.LowDatarateOptimize = 0x01;
    }
    else
    {
        SX1272.Settings.LoRa.LowDatarateOptimize = 0x00;
    }

    SX1272Write( REG_LR_MODEMCONFIG1,
                 ( SX1272Read( REG_LR_MODEMCONFIG1 ) &
                   RFLR_MODEMCONFIG1_BW_MASK &
                   RFLR_MODEMCONFIG1_CODINGRATE_MASK &
                   RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK &
                   RFLR_MODEMCONFIG1_RXPAYLOADCRC_MASK &
                   RFLR_MODEMCONFIG1_LOWDATARATEOPTIMIZE_MASK ) |
                   ( bandwidth << 6 ) | ( coderate << 3 ) |
                   ( 0/*fixLen*/ << 2 ) | ( crcOn << 1 ) |
                   SX1272.Settings.LoRa.LowDatarateOptimize );

    SX1272Write( REG_LR_MODEMCONFIG2,
                 ( SX1272Read( REG_LR_MODEMCONFIG2 ) &
                   RFLR_MODEMCONFIG2_SF_MASK &
                   RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) |
                   ( datarate << 4 ) |
                   ( ( symbTimeout >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) );

    SX1272Write( REG_LR_SYMBTIMEOUTLSB, ( uint8_t )( symbTimeout & 0xFF ) );

    SX1272Write( REG_LR_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
    SX1272Write( REG_LR_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );

    if( datarate == 6 )
    {
        SX1272Write( REG_LR_DETECTOPTIMIZE,
                     ( SX1272Read( REG_LR_DETECTOPTIMIZE ) &
                       RFLR_DETECTIONOPTIMIZE_MASK ) |
                       RFLR_DETECTIONOPTIMIZE_SF6 );
        SX1272Write( REG_LR_DETECTIONTHRESHOLD,
                     RFLR_DETECTIONTHRESH_SF6 );
    }
    else
    {
        SX1272Write( REG_LR_DETECTOPTIMIZE,
                     ( SX1272Read( REG_LR_DETECTOPTIMIZE ) &
                     RFLR_DETECTIONOPTIMIZE_MASK ) |
                     RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
        SX1272Write( REG_LR_DETECTIONTHRESHOLD,
                     RFLR_DETECTIONTHRESH_SF7_TO_SF12 );
    }
}



uint32_t SX1272GetTimeOnAir( uint8_t pktLen )
{
    uint32_t airTime = 0;

            double bw = 0.0;
            switch( SX1272.Settings.LoRa.Bandwidth )
            {
            case 0: // 125 kHz
                bw = 125e3;
                break;
            case 1: // 250 kHz
                bw = 250e3;
                break;
            case 2: // 500 kHz
                bw = 500e3;
                break;
            }

            // Symbol rate : time for one symbol (secs)
            double rs = bw / ( 1 << SX1272.Settings.LoRa.Datarate );
            double ts = 1 / rs;
            // time of preamble
            double tPreamble = ( SX1272.Settings.LoRa.PreambleLen + 4.25 ) * ts;
            // Symbol length of payload and time
            double tmp = ceil( ( 8 * pktLen - 4 * SX1272.Settings.LoRa.Datarate +
                                 28 + 16 * SX1272.Settings.LoRa.CrcOn -
                                 ( false/*SX1272.Settings.LoRa.FixLen*/ ? 20 : 0 ) ) /
                                 ( double )( 4 * ( SX1272.Settings.LoRa.Datarate -
                                 ( ( SX1272.Settings.LoRa.LowDatarateOptimize > 0 ) ? 2 : 0 ) ) ) ) *
                                 ( SX1272.Settings.LoRa.Coderate + 4 );
            double nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );
            double tPayload = nPayload * ts;
            // Time on air
            double tOnAir = tPreamble + tPayload;
            // return ms secs
            airTime = floor( tOnAir * 1e3 + 0.999 );
    return airTime;
}

void SX1272Send( uint8_t *buffer, uint8_t size )
{
            if( SX1272.Settings.LoRa.IqInverted == true )
            {
                SX1272Write( REG_LR_INVERTIQ, ( ( SX1272Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_ON ) );
                SX1272Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
            }
            else
            {
                SX1272Write( REG_LR_INVERTIQ, ( ( SX1272Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
                SX1272Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
            }

            // Initializes the payload size
            SX1272Write( REG_LR_PAYLOADLENGTH, size );

            // Full buffer used for Tx
            SX1272Write( REG_LR_FIFOTXBASEADDR, 0 );
            SX1272Write( REG_LR_FIFOADDRPTR, 0 );

            // FIFO operations can not take place in Sleep mode
            if( ( SX1272Read( REG_OPMODE ) & ~RF_OPMODE_MASK ) == RF_OPMODE_SLEEP )
            {
                SX1272SetStby( );
                DelayMs( 1 );
            }
            // Write payload buffer
            SX1272WriteFifo( buffer, size );

            SX1272Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                              RFLR_IRQFLAGS_RXDONE |
                                              RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                              RFLR_IRQFLAGS_VALIDHEADER |
                                              //RFLR_IRQFLAGS_TXDONE |
                                              RFLR_IRQFLAGS_CADDONE |
                                              RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                              RFLR_IRQFLAGS_CADDETECTED );

            // DIO0=TxDone
            SX1272Write( REG_DIOMAPPING1, ( SX1272Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_01 );

    SX1272.Settings.State = RF_TX_RUNNING;
    SX1272SetOpMode( RF_OPMODE_TRANSMITTER );

}

void SX1272SetRx()
{
    if( SX1272.Settings.LoRa.IqInverted == true )
    {
        SX1272Write( REG_LR_INVERTIQ, ( ( SX1272Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_OFF ) );
        SX1272Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON );
    }
    else
    {
        SX1272Write( REG_LR_INVERTIQ, ( ( SX1272Read( REG_LR_INVERTIQ ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK ) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF ) );
        SX1272Write( REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF );
    }

    SX1272Write( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |    // no RXtimeout in continuous mode
                                      //RFLR_IRQFLAGS_RXDONE |
                                      //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                      //RFLR_IRQFLAGS_VALIDHEADER |
                                      RFLR_IRQFLAGS_TXDONE |
                                      RFLR_IRQFLAGS_CADDONE |
                                      RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                      RFLR_IRQFLAGS_CADDETECTED );

    // DIO0=RxDone
    SX1272Write( REG_DIOMAPPING1, ( SX1272Read( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO0_MASK ) | RFLR_DIOMAPPING1_DIO0_00 );

    SX1272Write( REG_LR_FIFORXBASEADDR, 0 );
    SX1272Write( REG_LR_FIFOADDRPTR, 0 );

    SX1272.Settings.State = RF_RX_RUNNING;
    SX1272SetOpMode( RFLR_OPMODE_RECEIVER );
}



void SX1272SetSleep()
{
    SX1272SetOpMode( RF_OPMODE_SLEEP );
    SX1272.Settings.State = RF_IDLE;
}

void SX1272SetStby()
{
    SX1272SetOpMode( RF_OPMODE_STANDBY );
    SX1272.Settings.State = RF_IDLE;
}


void SX1272Reset( void )
{
    // Configure RESET as input
    GpioInit( &SX1272.Reset, RADIO_RESET, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );
    DelayMs( 10 );

    // Set RESET pin to 1
    GpioInit( &SX1272.Reset, RADIO_RESET, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );

    // Wait 1 ms
    DelayMs( 3 );

    // Configure RESET as input
    GpioInit( &SX1272.Reset, RADIO_RESET, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );

    // Wait 10 ms
    DelayMs( 10 );
}


void SX1272SetModem( RadioModems_t modem )
{
    RadioModems_t curr;

    if( ( SX1272Read( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_ON ) != 0 )
    {
        curr = MODEM_LORA;
    }
    else
    {
        curr = MODEM_FSK;
    }

    if( curr == modem )
    {
        return;
    }

    if ( modem == MODEM_LORA )
       {
         SX1272SetSleep( );
         SX1272Write( REG_OPMODE, ( SX1272Read( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );

         SX1272Write( REG_DIOMAPPING1, 0x00 );
         SX1272Write( REG_DIOMAPPING2, 0x00 );
       }
    else
       {
         SX1272SetSleep( );
         SX1272Write( REG_OPMODE, ( SX1272Read( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF );

         SX1272Write( REG_DIOMAPPING1, 0x00 );
         SX1272Write( REG_DIOMAPPING2, 0x30 ); // DIO5=ModeReady
       }
}

void SX1272Write( uint8_t addr, uint8_t data )
{
    SX1272WriteBuffer( addr, &data, 1 );
}

uint8_t SX1272Read( uint8_t addr )
{
    uint8_t data;
    SX1272ReadBuffer( addr, &data, 1 );
    return data;
}

void SX1272WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    __disable_irq( );

    //NSS = 0;
    GpioWrite( &SX1272.Spi.Nss, 0 );

    SpiInOut( &SX1272.Spi, addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        SpiInOut( &SX1272.Spi, buffer[i] );
    }

    //NSS = 1;
    GpioWrite( &SX1272.Spi.Nss, 1 );

    __enable_irq( );
}

void SX1272ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    __disable_irq( );

    //NSS = 0;
    GpioWrite( &SX1272.Spi.Nss, 0 );

    SpiInOut( &SX1272.Spi, addr & 0x7F );

    for( i = 0; i < size; i++ )
    {
        buffer[i] = SpiInOut( &SX1272.Spi, 0 );
    }

    //NSS = 1;
    GpioWrite( &SX1272.Spi.Nss, 1 );

    __enable_irq( );
}

void SX1272WriteFifo( uint8_t *buffer, uint8_t size )
{
    SX1272WriteBuffer( 0, buffer, size );
}

void SX1272ReadFifo( uint8_t *buffer, uint8_t size )
{
    SX1272ReadBuffer( 0, buffer, size );
}


extern bool switch_rx_tx;  //!!!!!!!!!!!!


void SX1272SetOpMode( uint8_t opMode )
{
    if( opMode == RF_OPMODE_SLEEP )
    {
      GpioWrite( &SX1272.AntRx, 0 );
      GpioWrite( &SX1272.AntTx, 0 );
    }
    else
    {
      if ( opMode == RFLR_OPMODE_TRANSMITTER )
         {
           GpioWrite( &SX1272.AntRx, switch_rx_tx ? 1 : 0 );
           GpioWrite( &SX1272.AntTx, switch_rx_tx ? 0 : 1 );
         }
      else
         {
           GpioWrite( &SX1272.AntRx, switch_rx_tx ? 0 : 1 );
           GpioWrite( &SX1272.AntTx, switch_rx_tx ? 1 : 0 );
         }
    }

    SX1272Write( REG_OPMODE, ( SX1272Read( REG_OPMODE ) & RF_OPMODE_MASK ) | opMode );
}


int SX1272GetVersion()
{
  SX1272SetModem( MODEM_FSK );

  int version = 0;
  int cnt = 0;
  while ((version == 0 || version == 0xff) && cnt < 100)
  {
    version = SX1272Read( REG_VERSION );
    DelayMs(1);
    cnt++;
  }

  return version;
}



void EXTI3_IRQHandler()
{
  if ( EXTI_GetITStatus(EXTI_Line3) != RESET )
     {
       EXTI_ClearITPendingBit(EXTI_Line3);

       if ( SX1272.Settings.State == RF_TX_RUNNING )
       {
         SX1272Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE );
         SX1272.Settings.State = RF_IDLE;
         OnTxDone( );
       }
       else
       if ( SX1272.Settings.State == RF_RX_RUNNING )
       {
         SX1272Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE );

         if( ( SX1272Read( REG_LR_IRQFLAGS ) & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK ) == RFLR_IRQFLAGS_PAYLOADCRCERROR )
         {
             SX1272Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR );
             OnRxError( );
         }
         else
         {
           int PktSnr = (int)(int8_t)SX1272Read( REG_LR_PKTSNRVALUE );
           float snr = PktSnr*0.25;

           int PktRssi = (int)(int8_t)SX1272Read( REG_LR_PKTRSSIVALUE );
           float rssi = -139.0 + PktRssi*1.0667;
           if( snr < 0 )
           {
               rssi += snr;
           }

           unsigned Size = SX1272Read( REG_LR_RXNBBYTES );
           uint8_t RxBuffer[256];
           memset(RxBuffer,0,sizeof(RxBuffer));
           SX1272ReadFifo( RxBuffer, Size );

           OnRxDone( RxBuffer, Size, rssi, snr );
         }
       }
     }
}
