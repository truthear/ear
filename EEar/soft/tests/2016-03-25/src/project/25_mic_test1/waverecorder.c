/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4_discovery.h"
#include "pdm_filter.h"
#include "waverecorder.h" 
#include <stddef.h>
#include <string.h>


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SPI2_DR_ADRESS 0x4000380C

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/


/* Current state of the audio recorder interface intialization */
static uint32_t AudioRecInited = 0;
PDMFilter_InitStruct Filter;

volatile int16_t RecBuf0[PCM_OUT_SIZE]; //buffer for filtered PCM data from MIC
volatile int16_t RecBuf1[PCM_OUT_SIZE]; //buffer for filtered PCM data from MIC
volatile uint8_t buffer_ready = 1;//number of buffer with fitered PCM data

volatile uint16_t Mic_DMA_PDM_Buffer0[PDM_BUFF_SIZE];//buffer for RAW MIC data (filled by DMA)
volatile uint16_t Mic_DMA_PDM_Buffer1[PDM_BUFF_SIZE];//buffer for RAW MIC data (filled by DMA)


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/



void DMA1_Stream3_IRQHandler(void)
{
  static uint16_t Mic_PDM_Buffer[PDM_BUFF_SIZE];//tmp buffer for HTONS
  uint8_t *dest;
  const uint8_t *src;
  uint32_t i;
  uint16_t* write_buf;//pointer for RAW data which must be filtered
  uint16_t* decode_buf;//pointer for filtered PCM data
  u16 MicGain = 64;//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< GAIN
  uint8_t tmp_buf_number;
  //unsigned t1,t2;

  if (DMA_GetFlagStatus(DMA1_Stream3, DMA_FLAG_TCIF3) != RESET)
  {
    DMA_ClearFlag(DMA1_Stream3, DMA_FLAG_TCIF3);

    if ((DMA1_Stream3->CR & DMA_SxCR_CT) == 0)//get number of current buffer
    {
      write_buf = (uint16_t*)Mic_DMA_PDM_Buffer1;
      decode_buf = (uint16_t*)RecBuf1;
      tmp_buf_number = 1;
    }
    else
    {
      write_buf = (uint16_t*)Mic_DMA_PDM_Buffer0;
      decode_buf = (uint16_t*)RecBuf0;
      tmp_buf_number = 0;
    }


    STM_EVAL_LEDToggle(LED5);

    //for (i=0;i<PDM_BUFF_SIZE;i++){Mic_PDM_Buffer[i] = HTONS(write_buf[i]);}//swap bytes for filter
    dest = (uint8_t*)Mic_PDM_Buffer;
    src = (uint8_t*)write_buf;
    i = PDM_BUFF_SIZE;
    do {
      uint8_t c1 = *src++;
      uint8_t c2 = *src++;
      *dest++ = c2;
      *dest++ = c1;
    } while ( --i );
    
    PDM_Filter_64_LSB((uint8_t*)Mic_PDM_Buffer, decode_buf, MicGain , (PDMFilter_InitStruct *)&Filter);//filter RAW data
    
    buffer_ready = tmp_buf_number;

    {
      int sum = 0, n;
    for ( n = 0; n < PCM_OUT_SIZE; n++ )
          {
            int s = (int)(short)decode_buf[n];
            if (s<0)
              s = -s;
            sum += s;
          }
    sum /= PCM_OUT_SIZE;

if ( sum > 1000 )    
  STM_EVAL_LEDOn(LED3);
else
  STM_EVAL_LEDOff(LED3);
   
    }


  }
}





void simple_rec_start(int freq)
{
  WaveRecorderInit(freq, 16, 1);
  WaveRecorderStart();
}

/**
  * @brief  Initialize wave recording
  * @param  AudioFreq: Sampling frequency
  *         BitRes: Audio recording Samples format (from 8 to 16 bits)
  *         ChnlNbr: Number of input microphone channel
  * @retval None
  */
uint32_t WaveRecorderInit(uint32_t AudioFreq, uint32_t BitRes, uint32_t ChnlNbr)
{ 
  /* Check if the interface is already initialized */
  if (AudioRecInited)
  {
    /* No need for initialization */
    return 0;
  }
  else
  {
    /* Enable CRC module */
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    
    /* Filter LP & HP Init */
    Filter.LP_HZ = 8000.0;
    Filter.HP_HZ = 10.0;

    Filter.Fs = AudioFreq;
    Filter.Out_MicChannels = 1;
    Filter.In_MicChannels = 1;
    
    PDM_Filter_Init((PDMFilter_InitStruct *)&Filter);
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    WaveRecorder_GPIO_Init();
    WaveRecorder_SPI_Init(AudioFreq);
    WaveRecorder_DMA_Init();
    
    /* Set state of the audio recorder to initialized */
    AudioRecInited = 1;
    
    return 0;
  }  
}

/**
  * @brief  Start audio recording
  * @param  pbuf: pointer to a buffer
  *         size: Buffer size
  * @retval None
  */
uint8_t WaveRecorderStart()
{
/* Check if the interface has already been initialized */
  if (AudioRecInited)
  {
    /* Enable the SPI peripheral */
    I2S_Cmd(SPI2, ENABLE); 
   
    return 0;
  }
  else
  {
    return 1; /* Cannot perform operation */
  }
}



static void WaveRecorder_DMA_Init(void)
{
  DMA_InitTypeDef DMA_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
  DMA_Cmd(DMA1_Stream3, DISABLE);//stream3 - spi2_rx
  DMA_DeInit(DMA1_Stream3);
  
  DMA_InitStructure.DMA_Channel = DMA_Channel_0; //spi2_rx
  DMA_InitStructure.DMA_PeripheralBaseAddr = SPI2_DR_ADRESS;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)Mic_DMA_PDM_Buffer0;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = (uint32_t)PDM_BUFF_SIZE;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//16bit
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;//<<<<<<<<<<<<
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA1_Stream3, &DMA_InitStructure);
  
  DMA_DoubleBufferModeConfig(DMA1_Stream3, (uint32_t)&Mic_DMA_PDM_Buffer1, DMA_Memory_0);//0 buffer is the first
  
  DMA_ITConfig(DMA1_Stream3, DMA_IT_TC, ENABLE);
  DMA_DoubleBufferModeCmd(DMA1_Stream3, ENABLE);
  
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  DMA_Cmd(DMA1_Stream3, ENABLE);
}


/**
  * @brief  Initialize GPIO for wave recorder.
  * @param  None
  * @retval None
  */
static void WaveRecorder_GPIO_Init(void)
{  
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIO clocks */
  RCC_AHB1PeriphClockCmd(SPI_SCK_GPIO_CLK, ENABLE);
  RCC_AHB1PeriphClockCmd(SPI_MOSI_GPIO_CLK, ENABLE);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  /* SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN;
  GPIO_Init(SPI_SCK_GPIO_PORT, &GPIO_InitStructure);
  
  /* Connect SPI pins to AF5 */  
  GPIO_PinAFConfig(SPI_SCK_GPIO_PORT, SPI_SCK_SOURCE, SPI_SCK_AF);
  
  /* SPI MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  SPI_MOSI_PIN;
  GPIO_Init(SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(SPI_MOSI_GPIO_PORT, SPI_MOSI_SOURCE, SPI_MOSI_AF);
}

/**
  * @brief  Initialize SPI peripheral.
  * @param  Freq :Audio frequency
  * @retval None
  */
static void WaveRecorder_SPI_Init(uint32_t Freq)
{
  I2S_InitTypeDef I2S_InitStructure;

  /* Enable the SPI clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);
  
  /* SPI configuration */
  SPI_I2S_DeInit(SPI2);
  I2S_InitStructure.I2S_AudioFreq = Freq*2;//i2s two channel mode
  I2S_InitStructure.I2S_Standard = I2S_Standard_LSB;
  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_High;
  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterRx;
  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
  /* Initialize the I2S peripheral with the structure above */
  I2S_Init(SPI2, &I2S_InitStructure);

  SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Rx, ENABLE);//enable DMA
}


/**
  * @brief  Stop audio recording
  * @param  None
  * @retval None
  */
uint32_t WaveRecorderStop(void)
{
  /* Check if the interface has already been initialized */
  if (AudioRecInited)
  {
    I2S_Cmd(SPI2, DISABLE); 
    TIM_Cmd(TIM6, DISABLE);
    return 0;
  }
  else
  {
    return 1;
  }
}


void simple_rec_stop()
{
  WaveRecorderStop();
}
