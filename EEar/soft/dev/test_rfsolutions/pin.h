
#ifndef __PIN_H__
#define __PIN_H__



class CPin
{
  public:
         enum EPins
         {
             PA_0 = 0, PA_1, PA_2, PA_3, PA_4, PA_5, PA_6, PA_7, PA_8, PA_9, PA_10, PA_11, PA_12, PA_13, PA_14, PA_15, 
             PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7, PB_8, PB_9, PB_10, PB_11, PB_12, PB_13, PB_14, PB_15,     
             PC_0, PC_1, PC_2, PC_3, PC_4, PC_5, PC_6, PC_7, PC_8, PC_9, PC_10, PC_11, PC_12, PC_13, PC_14, PC_15,     
             PD_0, PD_1, PD_2, PD_3, PD_4, PD_5, PD_6, PD_7, PD_8, PD_9, PD_10, PD_11, PD_12, PD_13, PD_14, PD_15,     
             PE_0, PE_1, PE_2, PE_3, PE_4, PE_5, PE_6, PE_7, PE_8, PE_9, PE_10, PE_11, PE_12, PE_13, PE_14, PE_15,     
         };
         
         static void InitAsInput(EPins pin,GPIOPuPd_TypeDef pupd=GPIO_PuPd_NOPULL);
         static void InitAsOutput(EPins pin,int init_value,GPIOPuPd_TypeDef pupd=GPIO_PuPd_NOPULL,GPIOOType_TypeDef otype=GPIO_OType_PP,GPIOSpeed_TypeDef speed=GPIO_Fast_Speed);
         static void InitAsAF(EPins pin,uint8_t af,GPIOPuPd_TypeDef pupd=GPIO_PuPd_NOPULL,GPIOOType_TypeDef otype=GPIO_OType_PP,GPIOSpeed_TypeDef speed=GPIO_Fast_Speed);

         static void Set(EPins pin);
         static void Reset(EPins pin);
         static void Toggle(EPins pin);
         static void SetValue(EPins pin,int value);
         static int GetValue(EPins pin);

         typedef void (*TCALLBACK)(void *parm);
         static void SetInterrupt(EPins pin,TCALLBACK cb,void *cb_parm=NULL,EXTITrigger_TypeDef trigger=EXTI_Trigger_Rising,uint8_t priority=15);

  private:
         typedef struct {
          TCALLBACK cb;
          void *cb_parm;
         } TCBPARM;
         
         static TCBPARM cb_list[16];

         static GPIO_TypeDef* GetPort(EPins pin);
         static uint32_t GetAHB1Periph(EPins pin);
         static uint32_t GetPinIndex(EPins pin);
         static uint8_t GetPinSource(EPins pin);
         static void EnablePower(EPins pin);

  protected:
         friend class CEXTIHandler;
         static void ExecuteCallback(int idx);
};



#endif
