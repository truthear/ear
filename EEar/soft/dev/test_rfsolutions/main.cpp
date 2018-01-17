
#include "include.h"



volatile bool recv_packet = false;
volatile bool recv_err = false;

unsigned recv_size = 0;
float recv_rssi = 0;
float recv_snr = 0;
uint8_t recv_data[256] = {0,};

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


volatile bool on_button = false;

// IRQ call!
void OnButton(void*)
{
  on_button = true;
}


CLED *LedGreen = NULL;
CLED *LedOrange = NULL;
CLED *LedBlue = NULL;
CLED *LedRed = NULL;
CButton *btn1 = NULL;



void ProcessAsSender(CLoraMote &lora)
{
  static int g_packet_num = 0;
  static int g_errors = 0;

  char Buffer[41];

  printf("------------\n");
  printf("Switching to sender mode\n");
  printf("------------\n");

  while ( !on_button )
  {
    sprintf(Buffer,"RFSolutions %06d_____________________R",g_packet_num);

    LedGreen->On();

    if ( !lora.Send(Buffer,sizeof(Buffer)-1,3000) )
       {
         printf("Timeout sending!\n");
         LedRed->On();
         g_errors++;
       }
    else
       {
         printf("Packet: %d, Errors: %d\n",g_packet_num,g_errors);
       }

    Sleep(50);
    LedGreen->Off();

    Sleep(900);

    g_packet_num++;
  }
}


void ProcessAsReceiver(CLoraMote &lora)
{
  static int g_packets = 0;
  static int g_errors = 0;
  
  printf("------------\n");
  printf("Switching to receiver mode\n");
  printf("------------\n");

  lora.StartReceiverMode(OnReceive,NULL);

  printf("Awaiting packets...\n");

  while( !on_button )
  {
    recv_err = false;
    recv_packet = false;
    while ( !recv_err && !recv_packet && !on_button ) {}

    if ( recv_err )
       {
         LedRed->On();
         printf("recv error!\n");
         g_errors++;
       }
    else
    if ( recv_packet )
       {
         LedBlue->On();
         printf("%d: %d errors, RSSI: %.1f, SNR: %.1f, [%d bytes]: ",g_packets,g_errors,recv_rssi,recv_snr,recv_size);
         for ( unsigned n = 0; n < recv_size; n++ )
         {
           printf("%c",recv_data[n]);
         }
         printf("\n");
         Sleep(50);
         LedBlue->Off();
         g_packets++;
       }
  }
}


void PostButtonAction()
{
  LedOrange->On();
  Sleep(500);
  LedOrange->Off();

  on_button = false;
}


int main()
{
  CSysTicks::Init();
  CCPUTicks::Init();

  LedGreen = new CLED(BOARD_LED_GREEN);
  LedOrange = new CLED(BOARD_LED_ORANGE);
  LedBlue = new CLED(BOARD_LED_BLUE);
  LedRed = new CLED(BOARD_LED_RED);
  btn1 = new CButton(BOARD_BUTTON1,OnButton);

  InitUART3();

  Sleep(1000);  // allow USB connect

  CLoraMote::TRadio radio;
  radio.freq = CLoraMote::RF_868MHz;
  radio.bw = CLoraMote::BW_500KHz;
  radio.sf = CLoraMote::SF_7;
  radio.cr = CLoraMote::CR_4_5;
  radio.crc = true;

  CBoardLoraMote lora(radio);

  while (1)
  {
    ProcessAsSender(lora);
    PostButtonAction();
    ProcessAsReceiver(lora);
    PostButtonAction();
  }
}



