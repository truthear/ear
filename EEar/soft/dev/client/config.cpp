
#include "include.h"



static bool BuildMap(const char *filename,std::map<std::string,std::string>& map)
{
  bool rc = false;
  
  FIL f;
  if ( f_open(&f,filename,FA_READ|FA_OPEN_EXISTING) == FR_OK )
     {
       unsigned fsize = f_size(&f);
       if ( fsize > 0 )
          {
            char* const buff = (char*)malloc(fsize+1);  // +1 for \n

            UINT rbytes = 0;
            if ( f_read(&f,buff,fsize,&rbytes) == FR_OK && rbytes == fsize )
               {
                 rc = true;  // OK!
                 
                 if ( buff[fsize-1] != '\n' )
                    {
                      buff[fsize] = '\n';  // append \n at end
                      fsize++;
                    }
                 
                 std::vector<std::string> list;
                 std::string s;

                 for ( unsigned n = 0; n < fsize; n++ )
                     {
                       char c = buff[n];
                       
                       if ( c != '\r' )
                          {
                            if ( c == '\n' )
                               {
                                 list.push_back(s);
                                 s.clear();
                               }
                            else
                               {
                                 s += c;
                               }
                          }
                     }

                 for ( unsigned n = 0; n < list.size(); n++ )
                     {
                       const std::string& s = list[n];

                       size_t idx = s.find('=');
                       if ( idx != std::string::npos )
                          {
                            std::string name = s.substr(0,idx);
                            std::string value = s.substr(idx+1);
                            if ( !name.empty() )
                               {
                                 map[name] = value;
                               }
                          }
                     }
               }

            free(buff);
          }

       f_close(&f);
     }

  return rc;
}


static void Get(unsigned char &_rc,const std::string& s)
{
  _rc = atoi(s.c_str());
}


static void Get(unsigned short &_rc,const std::string& s)
{
  _rc = atoi(s.c_str());
}


static void Get(int &_rc,const std::string& s)
{
  _rc = atoi(s.c_str());
}


static void Get(bool &_rc,const std::string& s)
{
  _rc = (atoi(s.c_str())!=0);
}


static void Get(std::string &_rc,const std::string& s)
{
  _rc = s;
}


bool ReadConfig(TCFG &cfg)
{
  bool rc = false;

  std::map<std::string,std::string> map;

  // must set ALL default values:
  map["device_id"]     = "65535";
  map["sector"]        = "255";
  map["port_tcp"]      = "0";
  map["port_udp"]      = "0";
  map["server"]        = "";
  map["ussd_balance"]  = "";
  map["sms_number"]    = "";
  map["sms_prefix"]    = "";
  map["gps_baud"]      = "0";
  map["use_gps"]       = "0";
  map["use_glonass"]   = "0";
  map["use_galileo"]   = "0";
  map["use_beidou"]    = "0";
  map["debug_mode"]    = "0";

  if ( BuildMap(CONFIG_FILENAME,map) )
     {
       Get(cfg.device_id    , map["device_id"]    );
       Get(cfg.sector       , map["sector"]       );
       Get(cfg.port_tcp     , map["port_tcp"]     );
       Get(cfg.port_udp     , map["port_udp"]     );
       Get(cfg.server       , map["server"]       );
       Get(cfg.ussd_balance , map["ussd_balance"] );
       Get(cfg.sms_number   , map["sms_number"]   );
       Get(cfg.sms_prefix   , map["sms_prefix"]   );
       Get(cfg.gps_baud     , map["gps_baud"]     );
       Get(cfg.use_gps      , map["use_gps"]      );
       Get(cfg.use_glonass  , map["use_glonass"]  );
       Get(cfg.use_galileo  , map["use_galileo"]  );
       Get(cfg.use_beidou   , map["use_beidou"]   );
       Get(cfg.debug_mode   , map["debug_mode"]   );

       // check for critical values
       if ( cfg.device_id != 0xFFFF && cfg.sector != 0xFF && cfg.gps_baud != 0 )
          {
            cfg.port_tcp = cfg.port_tcp ? cfg.port_tcp : DEFAULT_TCP_PORT;
            cfg.port_udp = cfg.port_udp ? cfg.port_udp : DEFAULT_UDP_PORT;
            cfg.server = cfg.server.empty() ? DEFAULT_SERVER : cfg.server;
            
            rc = true;
          }
     }

  return rc;
}

