
#include "include.h"



CPin::TCBPARM CPin::cb_list[16] = {{NULL,NULL},};


void CPin::ExecuteCallback(int idx)
{
  if ( idx >= 0 && idx <= 15 )
     {
       if ( cb_list[idx].cb )
          {
            cb_list[idx].cb(cb_list[idx].cb_parm);
          }
     }
}


GPIO_TypeDef* CPin::GetPort(EPins pin)
{
  GPIO_TypeDef* rc = NULL;

  switch ( (int)pin/16 )
  {
    case 0:
            rc = GPIOA;
            break;
    case 1:
            rc = GPIOB;
            break;
    case 2:
            rc = GPIOC;
            break;
    case 3:
            rc = GPIOD;
            break;
    case 4:
            rc = GPIOE;
            break;
  };

  return rc;
}


uint32_t CPin::GetAHB1Periph(EPins pin)
{
  uint32_t rc = 0xFFFFFFFF;

  switch ( (int)pin/16 )
  {
    case 0:
            rc = RCC_AHB1Periph_GPIOA;
            break;
    case 1:
            rc = RCC_AHB1Periph_GPIOB;
            break;
    case 2:
            rc = RCC_AHB1Periph_GPIOC;
            break;
    case 3:
            rc = RCC_AHB1Periph_GPIOD;
            break;
    case 4:
            rc = RCC_AHB1Periph_GPIOE;
            break;
  };

  return rc;
}


uint32_t CPin::GetPinIndex(EPins pin)
{
  return 1 << ((uint32_t)pin % 16);
}


uint8_t CPin::GetPinSource(EPins pin)
{
  return (uint8_t)((uint32_t)pin % 16);
}


void CPin::EnablePower(EPins pin)
{
  RCC_AHB1PeriphClockCmd(GetAHB1Periph(pin), ENABLE);
}


void CPin::InitAsInput(EPins pin,GPIOPuPd_TypeDef pupd)
{
  if ( pin != NC_PIN )
     {
       EnablePower(pin);

       GPIO_InitTypeDef i;
       i.GPIO_Pin = GetPinIndex(pin);
       i.GPIO_Mode = GPIO_Mode_IN;
       i.GPIO_Speed = GPIO_Fast_Speed;
       i.GPIO_OType = GPIO_OType_PP;
       i.GPIO_PuPd = pupd;
       GPIO_Init(GetPort(pin),&i);
     }
}


void CPin::InitAsOutput(EPins pin,int init_value,GPIOPuPd_TypeDef pupd,GPIOOType_TypeDef otype,GPIOSpeed_TypeDef speed)
{
  if ( pin != NC_PIN )
     {
       EnablePower(pin);

       GPIO_InitTypeDef i;
       i.GPIO_Pin = GetPinIndex(pin);
       i.GPIO_Mode = GPIO_Mode_OUT;
       i.GPIO_Speed = speed;
       i.GPIO_OType = otype;
       i.GPIO_PuPd = pupd;
       SetValue(pin,init_value);
       GPIO_Init(GetPort(pin),&i);
       SetValue(pin,init_value);
     }
}


void CPin::InitAsAF(EPins pin,uint8_t af,GPIOPuPd_TypeDef pupd,GPIOOType_TypeDef otype,GPIOSpeed_TypeDef speed)
{
  if ( pin != NC_PIN )
     {
       EnablePower(pin);

       GPIO_InitTypeDef i;
       i.GPIO_Pin = GetPinIndex(pin);
       i.GPIO_Mode = GPIO_Mode_AF;
       i.GPIO_Speed = speed;
       i.GPIO_OType = otype;
       i.GPIO_PuPd = pupd;
       GPIO_Init(GetPort(pin),&i);

       GPIO_PinAFConfig(GetPort(pin),GetPinSource(pin),af);
     }
}


void CPin::Set(EPins pin)
{
  if ( pin != NC_PIN )
     {
       GPIO_SetBits(GetPort(pin),GetPinIndex(pin));
     }
}


void CPin::Reset(EPins pin)
{
  if ( pin != NC_PIN )
     {
       GPIO_ResetBits(GetPort(pin),GetPinIndex(pin));
     }
}


void CPin::Toggle(EPins pin)
{
  if ( pin != NC_PIN )
     {
       GPIO_ToggleBits(GetPort(pin),GetPinIndex(pin));
     }
}


void CPin::SetValue(EPins pin,int value)
{
  if ( pin != NC_PIN )
     {
       if ( value )
          {
            Set(pin);
          }
       else
          {
            Reset(pin);
          }
     }
}


int CPin::GetValue(EPins pin)
{
  return pin != NC_PIN ? GPIO_ReadInputDataBit(GetPort(pin),GetPinIndex(pin)) : 0;
}


void CPin::SetInterrupt(EPins pin,TCALLBACK cb,void *cb_parm,EXTITrigger_TypeDef trigger,uint8_t priority)
{
  if ( pin != NC_PIN )
     {
       cb_list[GetPinSource(pin)].cb = cb;
       cb_list[GetPinSource(pin)].cb_parm = cb_parm;
         
       RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,ENABLE);
       
       SYSCFG_EXTILineConfig((uint32_t)pin/16,GetPinSource(pin));

       EXTI_InitTypeDef EXTI_InitStructure;
       EXTI_InitStructure.EXTI_Line = GetPinIndex(pin);
       EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
       EXTI_InitStructure.EXTI_Trigger = trigger;  
       EXTI_InitStructure.EXTI_LineCmd = ENABLE;
       EXTI_Init(&EXTI_InitStructure);

       uint8_t irqch = 0xFF;

       switch ( GetPinSource(pin) )
       {
         case 0:
                 irqch = EXTI0_IRQn;
                 break;
         case 1:
                 irqch = EXTI1_IRQn;
                 break;
         case 2:
                 irqch = EXTI2_IRQn;
                 break;
         case 3:
                 irqch = EXTI3_IRQn;
                 break;
         case 4:
                 irqch = EXTI4_IRQn;
                 break;
         case 5:
         case 6:
         case 7:
         case 8:
         case 9:
                 irqch = EXTI9_5_IRQn;
                 break;
         case 10:
         case 11:
         case 12:
         case 13:
         case 14:
         case 15:
                 irqch = EXTI15_10_IRQn;
                 break;
       };
       
       NVIC_InitTypeDef NVIC_InitStructure;
       NVIC_InitStructure.NVIC_IRQChannel = irqch;
       NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = priority;
       NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
       NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
       NVIC_Init(&NVIC_InitStructure); 
     }
}


class CEXTIHandler
{
  public:
          static void OnIRQ(int i_from,int i_to);
};


void CEXTIHandler::OnIRQ(int i_from,int i_to)
{
  for ( int n = i_from; n <= i_to; n++ )
      {
        uint32_t line = (1 << n);

        if ( EXTI_GetITStatus(line) != RESET )
           {
             EXTI_ClearITPendingBit(line);

             CPin::ExecuteCallback(n);
           }
      }
}


extern "C"
void EXTI0_IRQHandler()
{
  CEXTIHandler::OnIRQ(0,0);
}

extern "C"
void EXTI1_IRQHandler()
{
  CEXTIHandler::OnIRQ(1,1);
}

extern "C"
void EXTI2_IRQHandler()
{
  CEXTIHandler::OnIRQ(2,2);
}

extern "C"
void EXTI3_IRQHandler()
{
  CEXTIHandler::OnIRQ(3,3);
}

extern "C"
void EXTI4_IRQHandler()
{
  CEXTIHandler::OnIRQ(4,4);
}

extern "C"
void EXTI9_5_IRQHandler()
{
  CEXTIHandler::OnIRQ(5,9);
}

extern "C"
void EXTI15_10_IRQHandler()
{
  CEXTIHandler::OnIRQ(10,15);
}


