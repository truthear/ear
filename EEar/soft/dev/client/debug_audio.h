
#ifndef __DEBUG_AUDIO_H__
#define __DEBUG_AUDIO_H__



// Warning! FatFS should be initialized before!
class CDebugAudio
{
          static const unsigned BLOCK_SIZE = 4096;  // in bytes, heap allocated BLOCK_SIZE*2, max stack - BLOCK_SIZE
          
          unsigned m_1ms_samples;  // how much samples in 1 msec

          struct {
           char ar[BLOCK_SIZE*2];
           volatile unsigned wrpos;  // in bytes
           unsigned safe_read_part;  // 0 or 1
          } m_buff;

          struct {
           FIL handle;
           unsigned curr_block;    // current block index to write to, each block size is BLOCK_SIZE bytes
           unsigned total_blocks;  // total file size in blocks
          } m_file;

          bool init_ok;

  public:
          // file size more than 4GB is unsupported!!!
          CDebugAudio(int sample_rate,const char *filename,unsigned file_size_in_seconds,bool do_clear_at_init);
          ~CDebugAudio();

          void Push1ms(const short *samples);  // can be safe called from IRQ
          void Poll();
          void Stop();  // finalize file
};



#endif
