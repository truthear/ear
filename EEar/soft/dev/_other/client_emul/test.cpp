
#include "include.h"
#include "llxy.h"


/////////////////////////////

//#define SERVER_IP  "127.0.0.1"
#define SERVER_IP  "195.234.5.137"

const int max_random_delta_ms = 8;
const double total_zone_radius = 5000; // meters
const double sensors_zone_radius = 2000; // meters



double GetRandomCoordinate(double radius)
{
  return (rand() % (int)(radius*2)) - radius;
}

static double sqr(double x) { return x*x; }
static double dist(double x1,double y1,double x2,double y2) { return sqrt(sqr(x1-x2)+sqr(y1-y2)); }





const double lat0 = 77.1234567;
const double lon0 = 33.7654321;

CLLXYDeg llxy(lat0,lon0);


void UpdateFxFy(double& fx,double& fy)
{
  do {
   fx = GetRandomCoordinate(total_zone_radius);
   fy = GetRandomCoordinate(total_zone_radius);
  } while ( fabs(fx) < sensors_zone_radius || fabs(fy) < sensors_zone_radius );

  double lat,lon;
  llxy.GetLL(fx,fy,lat,lon);
  
  printf("fx = %.0f, fy = %.0f  [%.7f,%.7f]\n",fx,fy,lat,lon);
}


std::string EncodePacket(const void *sbuff,unsigned ssize)
{
  std::string rc;
  
  if ( ssize > 0 )
     {
       unsigned numblocks = (ssize+15)/16;
       unsigned align_size = numblocks*16;
       unsigned total_size = sizeof(unsigned)+align_size; // CRC32+16_aligned_packet

       char *p = (char*)alloca(total_size);  // not need to free

       // copy src packet
       memcpy(p+sizeof(unsigned),sbuff,ssize);

       // calc and save CRC32 of [packet+align_shit]:
       *(unsigned*)p = CRC32(0,(const unsigned char*)(p+sizeof(unsigned)),align_size);  

       // encode aligned data:
       AESCONTEXT ctx;
       aes_setkey_enc(&ctx,(const unsigned char*)OUR_AES_KEY,OUR_AES_KEY_SIZE);
       for ( unsigned n = 0; n < numblocks; n++ )
           {
             unsigned char *ioblock = (unsigned char*)(p+sizeof(unsigned)+n*16);
             aes_crypt_ecb_enc(&ctx,ioblock,ioblock);
           }

       // base64 of all data:
       rc = Base64Encode(p,total_size);
     }

  return rc;
}


void Send(int sector,int dev_id,double x,double y,double ts)
{
  TCmdFDetect cmd;

  cmd.header.cmd_id = CMDID_FDETECT;
  cmd.header.sector = sector;
  cmd.header.device_id = dev_id;
  cmd.header.client_ver = 0x100;
  cmd.header.fdetect_ver = 0x100;
  cmd.time_utc = (OURTIME)ts;

  double lat,lon;
  llxy.GetLL(x,y,lat,lon);
  
  cmd.geo.lat = GEO2INT(lat);
  cmd.geo.lon = GEO2INT(lon);

  cmd.fight_len_ms = 50 + (rand()%50);
  cmd.fight_db_amp = 16 + (double)(rand()%10)/10.;

  std::string packet = EncodePacket(&cmd,sizeof(cmd));

  SOCKET h_socket_udp = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(DEFAULT_UDP_PORT);
  addr.sin_addr.s_addr = inet_addr(SERVER_IP);
  sendto(h_socket_udp,packet.data(),packet.size(),0,(struct sockaddr*)&addr,sizeof(addr));
  shutdown(h_socket_udp,SD_BOTH);
  closesocket(h_socket_udp);
}


void main(int argc,char **argv)
{
  if ( argc != 2 )
     {
       printf("no sector specified!\n");
       getch();
       return;
     }

  WSADATA winsockdata;
  WSAStartup(MAKEWORD(2,2),&winsockdata);

  int sector = atoi(argv[1]);
  printf("sector: %d\n",sector);
     
  srand(1);

  SYSTEMTIME st;
  st.wYear = 2010;
  st.wMonth = 1;
  st.wDay = 10;
  st.wHour = 15;
  st.wMinute = 15;
  st.wSecond = 15;
  st.wMilliseconds = 15;
  
  double fts = (double)SystemTimeToOurTime(st);

  printf("Esc - exit, Enter - change fx,fy, 0..9 - send sensor data\n");

  double fx,fy;
  UpdateFxFy(fx,fy);
  
  do {

   int ch = getch();
   if ( ch == 27 )
    break;
   else
   if ( ch == 13 )
      {
        UpdateFxFy(fx,fy);
      }
   else
   if ( ch >= '0' && ch <= '9' )
      {
        int dev_id = sector*10+(ch-'0');

        double x = GetRandomCoordinate(sensors_zone_radius);
        double y = GetRandomCoordinate(sensors_zone_radius);
        double ts = fts+(dist(fx,fy,x,y)/SOUND_SPEED)*1000+((rand()%max_random_delta_ms)-max_random_delta_ms/2);

        Send(sector,dev_id,x,y,ts);
        printf("%d,%d: %.0f m dist, sent!\n",sector,dev_id,dist(x,y,fx,fy));
      }

  } while (1);

  WSACleanup();
}



