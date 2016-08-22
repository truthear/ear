
#include "include.h"



void ReadConfig(TCFG &cfg)
{
  cfg.device_id = 12;
  cfg.sector = 22;
  cfg.port_tcp = DEFAULT_TCP_PORT;
  cfg.port_udp = DEFAULT_UDP_PORT;
  cfg.server = "195.234.5.137";
  cfg.ussd_balance = "*111#";


}

