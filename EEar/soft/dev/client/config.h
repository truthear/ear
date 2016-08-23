
#ifndef __CONFIG_H__
#define __CONFIG_H__



typedef struct {
unsigned short device_id;
unsigned char sector;
int port_tcp;
int port_udp;
std::string server;
std::string ussd_balance;
std::string sms_number;
std::string sms_prefix;
int gps_baud;
bool use_gps;
bool use_glonass;
bool use_galileo;
bool use_beidou;
bool debug_mode;
} TCFG;


bool ReadConfig(TCFG &cfg);



#endif
