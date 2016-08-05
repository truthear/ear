
PROJ_NAME=project

CC=arm-none-eabi-gcc
CXX=arm-none-eabi-g++
OBJCOPY=arm-none-eabi-objcopy
OBJSIZE=arm-none-eabi-size

ifeq ($(OS),Windows_NT)
DEL_CMD=del /Q *.o $(PROJ_NAME).elf
BURN_CMD=stflash $(PROJ_NAME).bin 0x08000000
else
DEL_CMD=rm -f *.o $(PROJ_NAME).elf
BURN_CMD=../../reference/3rdParty/stlink/st-flash write $(PROJ_NAME).bin 0x08000000
endif

STM_COMMON=../STM32F4
CMSIS_COMMON=$(STM_COMMON)/Libraries/CMSIS
DRV_COMMON=$(STM_COMMON)/Libraries/STM32F4xx_StdPeriph_Driver
OUR_COMMON=../common

CSOURCES=
CSOURCES+=$(OUR_COMMON)/startup_stm32f40_41xxx.s
CSOURCES+=$(OUR_COMMON)/system_stm32f4xx.c
CSOURCES+=$(OUR_COMMON)/stm32f4xx_it.c
CSOURCES+=$(DRV_COMMON)/src/misc.c
CSOURCES+=$(DRV_COMMON)/src/stm32f4xx_dma.c
CSOURCES+=$(DRV_COMMON)/src/stm32f4xx_exti.c
CSOURCES+=$(DRV_COMMON)/src/stm32f4xx_gpio.c
CSOURCES+=$(DRV_COMMON)/src/stm32f4xx_pwr.c
CSOURCES+=$(DRV_COMMON)/src/stm32f4xx_rcc.c
CSOURCES+=$(DRV_COMMON)/src/stm32f4xx_rtc.c
CSOURCES+=$(DRV_COMMON)/src/stm32f4xx_sdio.c
CSOURCES+=$(DRV_COMMON)/src/stm32f4xx_spi.c
CSOURCES+=$(DRV_COMMON)/src/stm32f4xx_syscfg.c
CSOURCES+=$(DRV_COMMON)/src/stm32f4xx_tim.c
CSOURCES+=$(DRV_COMMON)/src/stm32f4xx_usart.c
CSOURCES+=$(wildcard *.c)

CXXSOURCES=$(wildcard *.cpp)

OBJS=$(addsuffix .o,$(basename $(notdir $(CSOURCES) $(CXXSOURCES))))

LIBS=

LDSCRIPT=$(OUR_COMMON)/stm32_flash.ld

INCLUDE_DIRS=-I$(OUR_COMMON) -I$(CMSIS_COMMON)/Include -I$(CMSIS_COMMON)/Device/ST/STM32F4xx/Include -I$(DRV_COMMON)/inc
HSE_VALUE=25000000
DEFS=-DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=$(HSE_VALUE) -DNDEBUG
MCFLAGS=-mcpu=cortex-m4 -mlittle-endian -mthumb -mthumb-interwork -mfloat-abi=softfp -mfpu=fpv4-sp-d16
WARNINGS=-Wall
CODEGENFLAGS=-Ofast -fno-strict-aliasing -fno-exceptions -fstack-check=no -fno-builtin
SPECS=--specs=nosys.specs

CFLAGS=-std=c99 $(INCLUDE_DIRS) $(DEFS) $(MCFLAGS) $(WARNINGS) $(CODEGENFLAGS) $(SPECS)
CXXFLAGS=-std=c++11 -fno-rtti $(INCLUDE_DIRS) $(DEFS) $(MCFLAGS) $(WARNINGS) $(CODEGENFLAGS) $(SPECS)
LDFLAGS=-Wl,--gc-sections -T$(LDSCRIPT) $(MCFLAGS) $(WARNINGS) $(SPECS)


all: proj cleanup burn

proj:
	$(CC) -c $(CFLAGS) $(CSOURCES)
ifneq ($(CXXSOURCES),)
	$(CXX) -c $(CXXFLAGS) $(CXXSOURCES)
	$(CXX) $(LDFLAGS) -o $(PROJ_NAME).elf $(OBJS) $(LIBS)
else
	$(CC) $(LDFLAGS) -o $(PROJ_NAME).elf $(OBJS) $(LIBS)
endif
	$(OBJSIZE) $(PROJ_NAME).elf
	$(OBJCOPY) -O binary $(PROJ_NAME).elf $(PROJ_NAME).bin

cleanup:
	$(DEL_CMD)

burn:
	$(BURN_CMD)


