
#include "include.h"




struct SQueue 
{
  static const unsigned MAX_QUEUE = 16;

  typedef struct {
  bool crc_err;
  uint8_t data[256];
  unsigned size;
  float rssi;
  float snr;
  } TPACKET;
  
  TPACKET ar[MAX_QUEUE];
  volatile unsigned to_write; // modified only by IRQ!
  volatile unsigned to_read;  // modified only by main thread!
 
  SQueue() : to_write(0), to_read(0) {}
  
  bool IsEmpty() const { return to_write == to_read; }
  void NextWrite() { SafeIncrement(to_write); }

 private:
  void SafeIncrement(volatile unsigned& v) 
  {
    unsigned t = v;
    t++;
    t = (t == MAX_QUEUE ? 0 : t);
    v = t;
  }
};

static SQueue g_q;


// IRQ call!
void OnReceive(void *parm,bool crc_error,const void *data,unsigned size,float rssi,float snr)
{
  SQueue::TPACKET& p = g_q.ar[g_q.to_write];

  if ( crc_error )
     {
       p.crc_err = true;
     }
  else
     {
       p.crc_err = false;
       memcpy(p.data,data,size);
       p.size = size;
       p.rssi = rssi;
       p.snr = snr;
     }

  g_q.NextWrite();
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

  char Buffer[256];

  printf("------------\n");
  printf("Switching to sender mode\n");
  printf("------------\n");

  while ( !on_button )
  {
    sprintf(Buffer,"RFSolutions %06d_____________________R",g_packet_num);

    LedGreen->On();

    if ( !lora.Send(Buffer,strlen(Buffer),3000) )
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


void ProcessReceivedPacket(const SQueue::TPACKET& p)
{
  static int g_packets = 0;
  static int g_errors = 0;
  
  if ( p.crc_err )
     {
       LedRed->On();
       printf("recv error!\n");
       g_errors++;
     }
  else
     {
       LedBlue->On();
       printf("%d: %d errors, RSSI: %.1f, SNR: %.1f, [%d bytes]: ",g_packets,g_errors,p.rssi,p.snr,p.size);
       for ( unsigned n = 0; n < p.size; n++ )
       {
         printf("%c",(int)p.data[n]);
       }
       printf("\n");
       Sleep(50);
       LedBlue->Off();
       g_packets++;
     }
}


void ProcessAsReceiver(CLoraMote &lora)
{
  printf("------------\n");
  printf("Switching to receiver mode\n");
  printf("------------\n");

  lora.StartReceiverMode(OnReceive,NULL);

  printf("Awaiting packets...\n");

  while( 1 )
  {
    while ( g_q.IsEmpty() && !on_button ) {}

    if ( on_button )
      break;

    unsigned last = g_q.to_write;
    unsigned first = g_q.to_read;

    for ( unsigned n = first; n != last; )
        {
          ProcessReceivedPacket(g_q.ar[n]);

          n++;
          if ( n == SQueue::MAX_QUEUE )
           n = 0;
        }

    g_q.to_read = last;
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
  InitUART2();

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



