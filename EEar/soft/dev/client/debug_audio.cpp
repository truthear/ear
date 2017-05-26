
#include "include.h"




CDebugAudio::CDebugAudio(int sample_rate,const char *filename,unsigned file_size_in_seconds,bool do_clear_at_init)
{
  assert(sample_rate>0);
  assert((sample_rate%1000)==0);
  
  m_filename = NNS(filename);
  
  m_1ms_samples = sample_rate/1000;

  assert((BLOCK_SIZE%(m_1ms_samples*sizeof(short)))==0);

  CLEAROBJ(m_buff.ar);
  m_buff.wrpos = 0;
  m_buff.safe_read_part = 1;

  m_file.curr_block = 0;
  QWORD max_size = (QWORD)file_size_in_seconds*((QWORD)sample_rate*sizeof(short));
  max_size = MIN(max_size,(QWORD)0xFFFFFFFF);
  unsigned max_blocks = max_size / BLOCK_SIZE;
  m_file.total_blocks = MAX(max_blocks,4);

  init_ok = false;

  if ( f_open(&m_file.handle,filename,FA_WRITE|FA_CREATE_ALWAYS) == FR_OK )
     {
       unsigned seek_pos = m_file.total_blocks*BLOCK_SIZE;
       if ( f_lseek(&m_file.handle,seek_pos) == FR_OK && f_tell(&m_file.handle) == seek_pos )
          {
            init_ok = true;

            f_lseek(&m_file.handle,0);

            if ( do_clear_at_init )
               {
                 for ( unsigned n = 0; n < m_file.total_blocks; n++ )
                     {
                       UINT wb = 0;
                       if ( f_write(&m_file.handle,m_buff.ar,BLOCK_SIZE,&wb) != FR_OK || wb != BLOCK_SIZE )
                          {
                            init_ok = false;
                            break;
                          }
                     }
                 
                 f_lseek(&m_file.handle,0);
               }

            if ( !init_ok )
               {
                 f_close(&m_file.handle);
               }
            else
               {
                 f_sync(&m_file.handle);  // flush changes
               }
          }
       else
          {
            f_close(&m_file.handle);
          }
     }

  if ( !init_ok )
     {
       f_unlink(filename);
     }
}


CDebugAudio::~CDebugAudio()
{
  if ( init_ok )
     {
       f_close(&m_file.handle);
     }
}


// should be safe called from IRQ!
void CDebugAudio::Push1ms(const short *samples)
{
  if ( samples )
     {
       volatile unsigned idx = m_buff.wrpos;

       unsigned num_bytes = m_1ms_samples*sizeof(*samples);
       const char *src = (const char*)samples;
       char *dst = m_buff.ar+idx;
       for ( unsigned n = 0; n < num_bytes; n++ )
           {
             *dst++ = *src++;
           }

       idx += num_bytes;
       assert(idx<=sizeof(m_buff.ar));
       idx = (idx == sizeof(m_buff.ar) ? 0 : idx);
       m_buff.wrpos = idx;
     }
}


void CDebugAudio::Poll()
{
  unsigned safe_read_part = (m_buff.wrpos >= sizeof(m_buff.ar)/2 ? 0 : 1);
  if ( m_buff.safe_read_part != safe_read_part )
     {
       m_buff.safe_read_part = safe_read_part;

       if ( init_ok )
          {
            char temp[BLOCK_SIZE];
            memcpy(temp,&m_buff.ar[safe_read_part*BLOCK_SIZE],BLOCK_SIZE);
            
            UINT wb = 0;
            bool wr_ok = (f_write(&m_file.handle,temp,BLOCK_SIZE,&wb) == FR_OK && wb == BLOCK_SIZE);
            
            m_file.curr_block++;
            if ( m_file.curr_block == m_file.total_blocks )
               {
                 m_file.curr_block = 0;
                 f_lseek(&m_file.handle,0);
               }

            if ( !wr_ok )
               {
                 f_close(&m_file.handle);
                 init_ok = false;

                 if ( f_open(&m_file.handle,m_filename.c_str(),FA_WRITE|FA_OPEN_EXISTING) == FR_OK )
                    {
                      f_lseek(&m_file.handle,m_file.curr_block*BLOCK_SIZE);
                      init_ok = true;
                    }
               }
          }
     }
}


void CDebugAudio::Stop()
{
  if ( init_ok )
     {
       // write marker and close file
       unsigned temp[BLOCK_SIZE/4];
       for ( unsigned n = 0; n < sizeof(temp)/sizeof(temp[0]); n++ )
           {
             temp[n] = 0x80007fff;
           }
       UINT wb = 0;
       f_write(&m_file.handle,temp,sizeof(temp),&wb);
       f_close(&m_file.handle);

       init_ok = false;
     }
}

