
#include "include.h"



volatile bool on_button1 = false;
volatile bool on_button2 = false;
volatile bool on_button3 = false;

// IRQ call!
void OnButton1(void*)
{
  on_button1 = true;
}

void OnButton2(void*)
{
  on_button2 = true;
}

void OnButton3(void*)
{
  on_button3 = true;
}


CLED *LedGreen = NULL;
CLED *LedOrange = NULL;
CLED *LedBlue = NULL;
CLED *LedRed = NULL;
CButton *btn1 = NULL;
CButton *btn2 = NULL;
CButton *btn3 = NULL;


volatile double g_lat = 0;
volatile double g_lon = 0;



void SendPacket(const CLoraMote::TRadio &radio,CLoraMote &lora,CLED *led)
{
  static const unsigned packet_size = 50;  // should be changed on receiver too!!!!
  static int g_packet_num = 0;
  static int g_num_errors = 0;
  static unsigned g_last_error_time = 0;

  char Buffer[256];

  for ( unsigned n = 0; n < sizeof(Buffer); n++ )
   Buffer[n] = 0;

  sprintf(Buffer,"%06d_%d_%dK_SF%d__%.6f_%.6f",g_packet_num,g_num_errors,(1<<(int)radio.bw)*125,(int)radio.sf,g_lat,g_lon);

  for ( unsigned n = 0; n < sizeof(Buffer); n++ )
   if ( Buffer[n] == 0 )
     Buffer[n] = '_';

  Buffer[packet_size-1] = '!';

  led->On();

  if ( g_last_error_time && GetTickCount() - g_last_error_time > 30000 )
     {
       //LedRed->Off();
       g_last_error_time = 0;
     }

  bool is_ok = lora.Send(Buffer,packet_size,0);

  if ( is_ok )
     {
       unsigned starttime = GetTickCount();
       unsigned time2wait = lora.GetTimeOnAirMs(packet_size) + 2000;

       while ( lora.IsSendingInProgress() && GetTickCount()-starttime<time2wait );
       
       is_ok = !lora.IsSendingInProgress();
     }

  if ( !is_ok )
     {
       //LedRed->On();
       g_last_error_time = GetTickCount();
       g_num_errors++;
     }
  else
    g_packet_num++;

  Sleep(50);
  led->Off();

}


void OnGPS(void *parm,OURTIME tim,double lat,double lon,short gnss)
{
  g_lat = lat;
  g_lon = lon;
}


void TrackGPS()
{
  if ( fabs(g_lat) > 0.00001 && fabs(g_lon) > 0.00001 )
     {
       LedRed->On();
     }
  else
     {
       LedRed->Off();
     }
}


int main()
{
  CSysTicks::Init();
  CCPUTicks::Init();

  LedGreen = new CLED(BOARD_LED_GREEN);
  LedOrange = new CLED(BOARD_LED_ORANGE);
  LedBlue = new CLED(BOARD_LED_BLUE);
  LedRed = new CLED(BOARD_LED_RED);
  btn1 = new CButton(BOARD_BUTTON1,OnButton1);
  btn2 = new CButton(BOARD_BUTTON2,OnButton2);
  btn3 = new CButton(BOARD_BUTTON3,OnButton3);

  Sleep(1000);  // allow USB connect

  new CBoardGPS(9600,OnGPS);

  while ( 1 )
  {
    if ( on_button1 || on_button2 || on_button3 )
      break;

    LedGreen->Toggle();
    LedOrange->Toggle();
    LedBlue->Toggle();

    TrackGPS();

    Sleep(100);
  }

  LedGreen->Off();
  LedOrange->Off();
  LedBlue->Off();


  CLoraMote::TRadio radio;
  radio.freq = CLoraMote::RF_868MHz;
  radio.bw = CLoraMote::BW_500KHz;  //125  500
  radio.sf = on_button1 ? CLoraMote::SF_7 : (on_button2 ? CLoraMote::SF_10 : CLoraMote::SF_12);
  radio.cr = CLoraMote::CR_4_5;
  radio.crc = true;

  CLED *led = on_button1 ? LedGreen : (on_button2 ? LedOrange : LedBlue);

  CBoardLoraMote lora(radio);

  while (1)
  {
    TrackGPS();
    SendPacket(radio,lora,led);
    Sleep(1000);
  }
}



