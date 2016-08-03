
#ifndef __SDCARD_H__
#define __SDCARD_H__


class CSDCard
{
          static bool b_init_ok;
  
  public:
          static const unsigned SECTOR_SIZE = 512;
          
          static bool InitCard(); // can be called many times (for hot-plug support)

          static uint64_t GetCardCapacity();  // in bytes
          static bool Read(void *buff,unsigned start_sector,unsigned num_sectors);
          static bool Write(const void *buff,unsigned start_sector,unsigned num_sectors);
};



#endif

