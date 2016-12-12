
#ifndef __CONFIG_H__
#define __CONFIG_H__


class CConfig 
{
  public:
          unsigned short device_id;
          unsigned char sector;
          int port_tcp;
          int port_udp;
          std::string server;
          std::string ussd_balance;
          std::string sms_number;
          std::string sms_prefix;
          std::string apn;
          int gps_baud;
          bool modem_old_firmware;
          bool use_gps;
          bool use_glonass;
          bool use_galileo;
          bool use_beidou;
          bool debug_mode;

  public:
          bool ReadConfig();

  private:
          static bool BuildMap(const char *filename,std::map<std::string,std::string>& map);
          static void Get(unsigned char &_rc,const std::string& s);
          static void Get(unsigned short &_rc,const std::string& s);
          static void Get(int &_rc,const std::string& s);
          static void Get(bool &_rc,const std::string& s);
          static void Get(std::string &_rc,const std::string& s);

};



#endif
