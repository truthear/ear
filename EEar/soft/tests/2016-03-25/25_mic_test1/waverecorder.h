/**
  ******************************************************************************
  * @file    Audio_playback_and_record/inc/waverecorder.h 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    28-October-2011
  * @brief   Header for waverecorder.c module
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2S_AUDIO_H
#define __I2S_AUDIO_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "main.h"


/* Exported types ------------------------------------------------------------*/
/* Exported Defines ----------------------------------------------------------*/
/* SPI Configuration defines */
#define SPI_SCK_PIN                       GPIO_Pin_13
#define SPI_SCK_GPIO_PORT                 GPIOB
#define SPI_SCK_GPIO_CLK                  RCC_AHB1Periph_GPIOB
#define SPI_SCK_SOURCE                    GPIO_PinSource13
#define SPI_SCK_AF                        GPIO_AF_SPI2

#define SPI_MOSI_PIN                      GPIO_Pin_14
#define SPI_MOSI_GPIO_PORT                GPIOB
#define SPI_MOSI_GPIO_CLK                 RCC_AHB1Periph_GPIOB
#define SPI_MOSI_SOURCE                   GPIO_PinSource14
#define SPI_MOSI_AF                       GPIO_AF_SPI2


#define PDM_BUFF_SIZE    64
#define PCM_OUT_SIZE     16


extern volatile int16_t RecBuf0[PCM_OUT_SIZE]; //buffer for filtered PCM data from MIC
extern volatile int16_t RecBuf1[PCM_OUT_SIZE]; //buffer for filtered PCM data from MIC
extern volatile uint8_t buffer_ready;//number of buffer with fitered PCM data



/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
uint32_t WaveRecorderInit(uint32_t AudioFreq, uint32_t BitRes, uint32_t ChnlNbr);
uint8_t WaveRecorderStart();
uint32_t WaveRecorderStop(void);
uint32_t WavaRecorderHeaderInit(uint8_t* pHeadBuf);
void WaveRecorderUpdate(void);
void simple_rec_start(int freq);
void simple_rec_stop();

static void WaveRecorder_GPIO_Init(void);
static void WaveRecorder_SPI_Init(uint32_t Freq);
static void WaveRecorder_DMA_Init(void);

#endif /* __WAVE_RECORDER_H */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
