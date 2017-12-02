#include "mbed.h"
#include "SDFileSystem.h"
#include <vector>

DigitalOut myled1(LED1);
DigitalOut myled2(LED2);
DigitalOut myled3(LED3);
DigitalOut myled4(LED4);
DigitalOut loraReset(PE_2);
DigitalOut rs485DE(PD_4,1);

DigitalIn  btn1(PD_9);
DigitalIn  btn2(PD_10);
DigitalIn  btn3(PD_11);
DigitalIn  pps(PC_13);

Serial pc (PB_10, PB_11,115200); // tx, rx
Serial gps(PB_6, PB_7,9600); // tx, rx
Serial gsm(PC_6, PC_7,57600); // tx, rx
Serial rs485(PD_5, PD_6,115200); // tx, rx 

SPI LoraSPI(PA_7, PA_6, PA_5);
DigitalOut LoraSpiCs(PA_4,1);

int printf(const char *pFormat, ...)
{
    int result = 0;

    va_list ap;
    va_start(ap, pFormat);

    char pStr[1024];
    vsprintf(pStr, pFormat, ap);

    const char *p = pStr;
    result = pc.printf("%s",pStr);
    va_end(ap);

    return result;
}
#if DEVICE_STDIO_MESSAGES
void error(const char* format, ...){
      int result = 0;

    va_list ap;
    va_start(ap, format);

    char pStr[1024];
    vsprintf(pStr, format, ap);

    const char *p = pStr;
    result = pc.printf("%s",pStr);
    va_end(ap);

    return;
}
#endif
int main() {
    printf("\n\rC++ Hello World  %x %d\n",0xdeadbeef,1234);

    loraReset.write(0);
    wait(1.0); //wait 1 sec 
    loraReset.write(1);

    gsm.attach([](){
      pc.putc(gsm.getc());
    });
 
    gps.attach([](){
      char ch = gps.getc();
      if(!btn2) {
        pc.putc(ch);
      }
    });

    rs485.attach([](){
      pc.putc(rs485.getc());
    });

    pc.attach([](){
      char ch = pc.getc();
      pc.putc(ch);
      if (ch == '#') gsm.printf("sys get ver\r\n"); else
      if (ch == '!') rs485.printf("test\n"); else  
      gsm.putc(ch);
    });
    
    printf("\nSPI send .. \n");
    LoraSpiCs = 0;
    LoraSPI.write(0x8F);
    printf("some register value  = 0x%x\n", LoraSPI.write(0x00)); 
    LoraSpiCs =1;
    printf("SPI finished .. \n");
    while(1) {
        myled1   = !btn1;
        myled2   = !btn2;
        myled3   = !btn3;
        myled4   = pps;
    }
}
