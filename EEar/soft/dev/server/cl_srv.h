
#ifndef __CL_SRV_H__
#define __CL_SRV_H__


// common client and server types and constants:

#define OUR_AES_KEY        "12653876977771827628128817812700"
#define OUR_AES_KEY_SIZE   256  // in bits

#define DEFAULT_TCP_PORT   4111
#define DEFAULT_UDP_PORT   4112

enum {
CMDID_SERVER_PING = 1,
CMDID_USSD_BALANCE = 2,
CMDID_FDETECT = 3,

};


// this struct should be at the beginning of any packet!!!
typedef struct {
unsigned char cmd_id;   // should be first byte in packet!
unsigned char sector;
unsigned short device_id;
unsigned short client_ver;
unsigned short fdetect_ver;
} TCmdHeader;

typedef struct {
int lat;      // use INT2GEO macro to convert to real coords
int lon;      // use INT2GEO macro to convert to real coords
} TGeo;

typedef struct {
TCmdHeader header;
OURTIME time_utc;
TGeo geo;
unsigned last_timesync_ms;  // how many msec ago was last time sync
} TCmdServerPing;

typedef struct {
TCmdHeader header;
char text[1];  // NULL terminated text
} TCmdUSSDBalance;

typedef struct {
TCmdHeader header;
OURTIME time_utc;   // event time
TGeo geo;
unsigned fight_len_ms;
float fight_db_amp;
} TCmdFDetect;



#define GEO2INT(g)     ((int)((double)(g)*(double)10000000.0))
#define INT2GEO(i)     ((double)(i)/(double)10000000.0)




#endif
