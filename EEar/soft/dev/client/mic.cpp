
#include "include.h"
#include "pdm/pdm_filter.h"



static const int PDM_DECIM_FACTOR = 64;  // do not change
static const int NUM_PCM_SAMPLES = CMic::SAMPLE_RATE/1000;  // must be 1 msec!
static const int NUM_PDM_DWORDS = NUM_PCM_SAMPLES*PDM_DECIM_FACTOR/8/4;

#define ARHTONS(ar,n)  { ar[n] = ((ar[n] >> 8) & 0x00FF00FF) | ((ar[n] & 0x00FF00FF) << 8); }

static uint32_t pdm_buff0[NUM_PDM_DWORDS];  // these buffers are filled by DMA by MSB words
static uint32_t pdm_buff1[NUM_PDM_DWORDS];  // these buffers are filled by DMA by MSB words
static PDMFilter_InitStruct m_filter;

CMic::TCALLBACK CMic::p_cb = NULL;
void* CMic::p_cbparm = NULL;



void CMic::Init(TCALLBACK cb,void *cbparm,int irq_priority)
{
  p_cb = cb;
  p_cbparm = cbparm;

  CLEAROBJ(pdm_buff0);
  CLEAROBJ(pdm_buff1);

  // turn on CRC before PDM_Filter_Init()
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC,ENABLE);
  
  // init PDM filter
  CLEAROBJ(m_filter);
  m_filter.Fs = SAMPLE_RATE;
  m_filter.LP_HZ = SAMPLE_RATE/2;
  m_filter.HP_HZ = 5;
  m_filter.In_MicChannels = 1;
  m_filter.Out_MicChannels = 1;
  PDM_Filter_Init(&m_filter);


  // GPIO init
  CPin::InitAsAF(CPin::PB_13,GPIO_AF_SPI2);
  CPin::InitAsAF(CPin::PB_15,GPIO_AF_SPI2);


  // SPI init
  //RCC_I2SCLKConfig(RCC_I2SBus_APB1,RCC_I2S2CLKSource_PLLI2S);
  RCC_PLLI2SCmd(ENABLE);
  while ( RCC_GetFlagStatus(RCC_FLAG_PLLI2SRDY) == RESET ) {}

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);
  
  SPI_I2S_DeInit(SPI2);

  I2S_InitTypeDef I2S_InitStructure;
  I2S_InitStructure.I2S_AudioFreq = SAMPLE_RATE*2;  // I2S two channel mode
  I2S_InitStructure.I2S_Standard = I2S_Standard_LSB;
  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_High;
  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterRx;
  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
  I2S_Init(SPI2, &I2S_InitStructure);

  SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Rx, ENABLE); // enable DMA
    

  // DMA init  
  DMA_Stream_TypeDef* DMAy_Streamx = DMA1_Stream3;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
  DMA_Cmd(DMAy_Streamx, DISABLE);
  DMA_DeInit(DMAy_Streamx);
  
  DMA_InitTypeDef DMA_InitStructure;
  DMA_InitStructure.DMA_Channel = DMA_Channel_0;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI2->DR));
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)(pdm_buff0);
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = NUM_PDM_DWORDS*2;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 16 bit
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMAy_Streamx, &DMA_InitStructure);
  
  DMA_DoubleBufferModeConfig(DMAy_Streamx, (uint32_t)(pdm_buff1), DMA_Memory_0); // 0 buffer is the first
  
  DMA_ITConfig(DMAy_Streamx, DMA_IT_TC, ENABLE);
  DMA_DoubleBufferModeCmd(DMAy_Streamx, ENABLE);
  
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = irq_priority;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  DMA_Cmd(DMAy_Streamx, ENABLE);

 
  // Start
  I2S_Cmd(SPI2,ENABLE); 
}


void CMic::OnIRQ_Internal(DMA_Stream_TypeDef* DMAy_Streamx,uint32_t it_flag)
{
  if ( DMA_GetITStatus(DMAy_Streamx,it_flag) != RESET )
     {
       DMA_ClearITPendingBit(DMAy_Streamx,it_flag);

       if ( p_cb )
          {
            const int gain = 64; // max
            int16_t pcm[NUM_PCM_SAMPLES];

            if ( DMA_GetCurrentMemoryTarget(DMAy_Streamx) == 1 )
               {
                 ARHTONS(pdm_buff0,0);   ARHTONS(pdm_buff0,1);   ARHTONS(pdm_buff0,2);   ARHTONS(pdm_buff0,3);
                 ARHTONS(pdm_buff0,4);   ARHTONS(pdm_buff0,5);   ARHTONS(pdm_buff0,6);   ARHTONS(pdm_buff0,7);
                 ARHTONS(pdm_buff0,8);   ARHTONS(pdm_buff0,9);   ARHTONS(pdm_buff0,10);  ARHTONS(pdm_buff0,11);
                 ARHTONS(pdm_buff0,12);  ARHTONS(pdm_buff0,13);  ARHTONS(pdm_buff0,14);  ARHTONS(pdm_buff0,15);
                 ARHTONS(pdm_buff0,16);  ARHTONS(pdm_buff0,17);  ARHTONS(pdm_buff0,18);  ARHTONS(pdm_buff0,19);
                 ARHTONS(pdm_buff0,20);  ARHTONS(pdm_buff0,21);  ARHTONS(pdm_buff0,22);  ARHTONS(pdm_buff0,23);
                 ARHTONS(pdm_buff0,24);  ARHTONS(pdm_buff0,25);  ARHTONS(pdm_buff0,26);  ARHTONS(pdm_buff0,27);
                 ARHTONS(pdm_buff0,28);  ARHTONS(pdm_buff0,29);  ARHTONS(pdm_buff0,30);  ARHTONS(pdm_buff0,31);

                 PDM_Filter_64_LSB((uint8_t*)pdm_buff0,(uint16_t*)pcm,gain,&m_filter);
               }
            else
               {
                 ARHTONS(pdm_buff1,0);   ARHTONS(pdm_buff1,1);   ARHTONS(pdm_buff1,2);   ARHTONS(pdm_buff1,3);
                 ARHTONS(pdm_buff1,4);   ARHTONS(pdm_buff1,5);   ARHTONS(pdm_buff1,6);   ARHTONS(pdm_buff1,7);
                 ARHTONS(pdm_buff1,8);   ARHTONS(pdm_buff1,9);   ARHTONS(pdm_buff1,10);  ARHTONS(pdm_buff1,11);
                 ARHTONS(pdm_buff1,12);  ARHTONS(pdm_buff1,13);  ARHTONS(pdm_buff1,14);  ARHTONS(pdm_buff1,15);
                 ARHTONS(pdm_buff1,16);  ARHTONS(pdm_buff1,17);  ARHTONS(pdm_buff1,18);  ARHTONS(pdm_buff1,19);
                 ARHTONS(pdm_buff1,20);  ARHTONS(pdm_buff1,21);  ARHTONS(pdm_buff1,22);  ARHTONS(pdm_buff1,23);
                 ARHTONS(pdm_buff1,24);  ARHTONS(pdm_buff1,25);  ARHTONS(pdm_buff1,26);  ARHTONS(pdm_buff1,27);
                 ARHTONS(pdm_buff1,28);  ARHTONS(pdm_buff1,29);  ARHTONS(pdm_buff1,30);  ARHTONS(pdm_buff1,31);

                 PDM_Filter_64_LSB((uint8_t*)pdm_buff1,(uint16_t*)pcm,gain,&m_filter);
               }

            p_cb(p_cbparm,pcm,NUM_PCM_SAMPLES);
          }
     }
}



extern "C"
void DMA1_Stream3_IRQHandler(void)
{
  CMic::OnIRQ_Internal(DMA1_Stream3,DMA_IT_TCIF3);
}





