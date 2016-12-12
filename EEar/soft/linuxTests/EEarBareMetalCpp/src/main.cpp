/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * STM32F4DIS Base Board RS-232 Interface is mapped to the UART6 of
  * STM32F4-Discovery Board (see STM32F4-BB User Manual).
  *
  * Note: Make sure that jumpers JP1 and JP2 are fitted on the Base Board.
  *
  * This example is based on http://developer.mbed.org/handbook/Serial
  *
  * @param  None
  * @retval None
  */
#include "mbed.h"
#include <vector>
using namespace mbed;
 
DigitalOut myled1(LED1);
DigitalOut myled2(LED2);
DigitalOut myled3(LED3);
DigitalOut myled4(LED4);
DigitalOut gsmReset(GSM_RESET,0);

DigitalIn  gsmPowerOk(GSM_PWR_OK);
DigitalIn  btn1(PD_9);
DigitalIn  btn2(PD_10);
DigitalIn  btn3(PD_11);
DigitalIn  pps(PC_13);

Serial debug (PB_10, PB_11); // tx, rx
Serial gps(PB_6, PB_7); // tx, rx
Serial gsm(PD_5, PD_6); // tx, rx
 


int main() {

    gsm.baud(9600);
    gps.baud(9600);
    debug.baud(115200);

    debug.printf("\n\rC++ Hello World1  %x",0xdeadbeef);

    gsm.attach([](){
      debug.putc(gsm.getc());
    });

    gps.attach([](){
      debug.putc(gps.getc());
    });

    debug.attach([](){
      if (debug.getc() == 'a') gsm.printf("AT\r"); // test for modem OK
    });

    while(1) {
        myled1   = !btn1;
        myled2   = !btn2;
        myled3   = !btn3;
        myled4   = pps;
        gsmReset = !btn1;

    }
}

/*
 * Override C++ new/delete operators to reduce memory footprint
 */
#ifdef CUSTOM_NEW

void *operator new(size_t size) {
        return malloc(size);
}

void *operator new[](size_t size) {
        return malloc(size);
}

void operator delete(void *p) {
        free(p);
}

void operator delete[](void *p) {
        free(p);
}

#endif
