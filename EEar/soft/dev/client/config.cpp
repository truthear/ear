
#include "include.h"



bool CConfig::BuildMap(const char *filename,std::map<std::string,std::string>& map)
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


void CConfig::Get(unsigned char &_rc,const std::string& s)
{
  _rc = atoi(s.c_str());
}


void CConfig::Get(unsigned short &_rc,const std::string& s)
{
  _rc = atoi(s.c_str());
}


void CConfig::Get(int &_rc,const std::string& s)
{
  _rc = atoi(s.c_str());
}


void CConfig::Get(bool &_rc,const std::string& s)
{
  _rc = (atoi(s.c_str())!=0);
}


void CConfig::Get(std::string &_rc,const std::string& s)
{
  _rc = s;
}


bool CConfig::ReadConfig()
{
  bool rc = false;

  std::map<std::string,std::string> map;

  // must set ALL default values:
  map["device_id"]          = "65535";
  map["sector"]             = "255";
  map["port_tcp"]           = "0";
  map["port_udp"]           = "0";
  map["server"]             = "";
  map["ussd_balance"]       = "";
  map["sms_number"]         = "";
  map["sms_prefix"]         = "";
  map["apn"]                = "";
  map["gps_baud"]           = "0";
  map["modem_old_firmware"] = "0";
  map["use_gps"]            = "0";
  map["use_glonass"]        = "0";
  map["use_galileo"]        = "0";
  map["use_beidou"]         = "0";
  map["debug_mode"]         = "0";

  if ( BuildMap(CONFIG_FILENAME,map) )
     {
       Get(device_id          , map["device_id"]           );
       Get(sector             , map["sector"]              );
       Get(port_tcp           , map["port_tcp"]            );
       Get(port_udp           , map["port_udp"]            );
       Get(server             , map["server"]              );
       Get(ussd_balance       , map["ussd_balance"]        );
       Get(sms_number         , map["sms_number"]          );
       Get(sms_prefix         , map["sms_prefix"]          );
       Get(apn                , map["apn"]                 );
       Get(gps_baud           , map["gps_baud"]            );
       Get(modem_old_firmware , map["modem_old_firmware"]  );
       Get(use_gps            , map["use_gps"]             );
       Get(use_glonass        , map["use_glonass"]         );
       Get(use_galileo        , map["use_galileo"]         );
       Get(use_beidou         , map["use_beidou"]          );
       Get(debug_mode         , map["debug_mode"]          );

       // check for critical values
       if ( device_id != 0xFFFF && sector != 0xFF && gps_baud != 0 )
          {
            port_tcp = port_tcp ? port_tcp : DEFAULT_TCP_PORT;
            port_udp = port_udp ? port_udp : DEFAULT_UDP_PORT;
            server = server.empty() ? DEFAULT_SERVER : server;
            
            rc = true;
          }
     }

  return rc;
}

