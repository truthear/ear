
#ifndef __MIC_H__
#define __MIC_H__


class CMic
{
  public:
          static const int SAMPLE_RATE = 16000;  // do not change!!!

          typedef void (*TCALLBACK)(void*,const int16_t* pcm_buff,int num_samples);

  private:        
          static TCALLBACK p_cb;
          static void *p_cbparm;

  public:
          static void Init(TCALLBACK cb,void *cbparm=NULL,int irq_priority=10);

  public:
          // used internally:
          static void OnIRQ_Internal(DMA_Stream_TypeDef* DMAy_Streamx,uint32_t it_flag);
};




#endif
