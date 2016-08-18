
#ifndef __CONFIG_H__
#define __CONFIG_H__



typedef struct {
unsigned short device_id;
unsigned char sector;
int port_tcp;
int port_udp;
std::string server;
std::string ussd_balance;

} TCFG;



void ReadConfig(TCFG &cfg);



#endif
