
#ifndef __CONFIG_H__
#define __CONFIG_H__


typedef struct {
int port_tcp;
int port_udp;

} TCFG;


void ReadConfig(TCFG &cfg);


#endif
