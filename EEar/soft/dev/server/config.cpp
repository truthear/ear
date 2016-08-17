
#include "include.h"



void ReadConfig(TCFG &cfg)
{
  char filename[MAX_PATH] = "";
  GetCurrentAppPath(filename);
  PathAppend(filename,OUR_CFG_FILENAME);

  const char *section = "Main";

  cfg.port_tcp = GetPrivateProfileInt(section,"port_tcp",0,filename);
  cfg.port_tcp = (cfg.port_tcp == 0 ? DEFAULT_TCP_PORT : cfg.port_tcp);

  cfg.port_udp = GetPrivateProfileInt(section,"port_udp",0,filename);
  cfg.port_udp = (cfg.port_udp == 0 ? DEFAULT_UDP_PORT : cfg.port_udp);


}

