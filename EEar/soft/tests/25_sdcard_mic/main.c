
#include "main.h"
#include "ff.h"
#include "waverecorder.h"



void delay_ms(uint32_t ms)
{
  volatile uint32_t nCount;
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  nCount = (RCC_Clocks.HCLK_Frequency / 10000) * ms;
  for(; nCount != 0; nCount --);
}

static void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Configure the NVIC Preemption Priority Bits */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

  NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = SD_SDIO_DMA_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_Init(&NVIC_InitStructure);  
}


extern volatile short final_buff[];
extern volatile int final_buff_idx;
extern const int final_buff_max;




int main(void)
{
  for ( int n = 0; n < final_buff_max; n++ )
  {
    final_buff[n] = 0;
  }

  delay_ms(300);
  
  STM_EVAL_LEDInit(LED1);
  STM_EVAL_LEDInit(LED2);
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);  

  NVIC_Configuration();

  /*------------------------------ SD Init ---------------------------------- */
  if ( SD_Init() != SD_OK )
     {
       STM_EVAL_LEDOn(LED4); 
     }
  else
     {
       if ( SD_Detect() != SD_PRESENT )
          {
            STM_EVAL_LEDOn(LED3); 
          }
       else
          {
            FIL f;
            FATFS ffs;
            
            f_mount(&ffs,"0:",1);

                 if ( f_open(&f,"0:out.raw",FA_WRITE|FA_CREATE_ALWAYS) == FR_OK )
                    {
                      int old_part = 1, safe_read_part;

                      RCC_PLLI2SCmd(ENABLE);
                      simple_rec_start(16000);
                      
                      STM_EVAL_LEDOn(LED1); 
                      
                      for ( int iteration = 0; iteration < 120; iteration++ )
                      {
                        while (1)
                        {
                          safe_read_part = (final_buff_idx >= final_buff_max/2 ? 0 : 1);
                          if ( safe_read_part != old_part )
                            break;
                        }
                        
                        {
                          UINT wb = 0;
                          f_write(&f,(void*)&final_buff[safe_read_part*(final_buff_max/2)],final_buff_max/2*sizeof(short),&wb);
                        }
                        
                        old_part = safe_read_part;
                      }

                      STM_EVAL_LEDOff(LED1); 
                      f_close(&f);
                    }
                 else
                    {
                      STM_EVAL_LEDOn(LED2); 
                    }
          }
     }

  while (1)
  {
  }
}

/////////////////

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
