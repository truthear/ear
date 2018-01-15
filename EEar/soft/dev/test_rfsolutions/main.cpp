
#include "include.h"


//#define SENDER


CLED LedGreen(BOARD_LED_GREEN);
CLED LedOrange(BOARD_LED_ORANGE);
CLED LedBlue(BOARD_LED_BLUE);
CLED LedRed(BOARD_LED_RED);

volatile bool recv_packet = false;
volatile bool recv_err = false;

unsigned recv_size = 0;
float recv_rssi = 0;
float recv_snr = 0;
uint8_t recv_data[256];


// IRQ call!
void OnReceive(void *parm,bool crc_error,const void *data,unsigned size,float rssi,float snr)
{
  if ( crc_error )
     {
       recv_packet = false;
       recv_err = true;
     }
  else
     {
       recv_size = size;
       recv_rssi = rssi;
       recv_snr = snr;
       memcpy(recv_data,data,size);
       recv_err = false;
       recv_packet = true;
     }
}



int main()
{
  CSysTicks::Init();
  CCPUTicks::Init();
  InitUART3();

  Sleep(500);  // allow USB connect

  CLoraMote::TRadio radio;
  radio.freq = CLoraMote::RF_868MHz;
  radio.bw = CLoraMote::BW_500KHz;
  radio.sf = CLoraMote::SF_7;
  radio.cr = CLoraMote::CR_4_5;
  radio.crc = true;

  CBoardLoraMote lora(radio);

#ifdef SENDER  
  char Buffer[41];

  printf("Expected time on air: %d msec\n",lora.GetTimeOnAirMs(sizeof(Buffer)-1));

  int packet_num = 0;
  while (1)
  {
    sprintf(Buffer,"RFSolutions %04d_______________________R",packet_num);

    LedGreen.On();

    unsigned starttime = GetTickCount();
    if ( !lora.Send(Buffer,sizeof(Buffer)-1,3000) )
       {
         printf("Timeout sending!\n");
         LedRed.On();
       }
    else
       {
         unsigned t1 = GetTickCount();
         while ( GetTickCount()-starttime<3000 && lora.IsSendingInProgress() ){}
         if ( lora.IsSendingInProgress() )
            {
              printf("Timeout sending!\n");
              LedRed.On();
            }
         else
            {
              unsigned endtime = GetTickCount();
              printf("Real time on air: %d msec (%d)\n",endtime-t1,t1-starttime);
            }
       }

    Sleep(50);
    LedGreen.Off();

    Sleep(950);

    packet_num++;
  }

#else
  lora.StartReceiverMode(OnReceive,NULL);

  while(1)
  {
    recv_err = false;
    recv_packet = false;
    while ( !recv_err && !recv_packet ) {}

    if ( recv_err )
       {
         LedRed.On();
         printf("recv error!\n");
       }
    else
       {
         LedBlue.On();
         printf("RSSI: %.1f, SNR: %.1f, [%d bytes]: ",recv_rssi,recv_snr,recv_size);
         for ( unsigned n = 0; n < recv_size; n++ )
         {
           printf("%c",recv_data[n]);
         }
         printf("\n");
         Sleep(50);
         LedBlue.Off();
       }
  }

#endif

}



