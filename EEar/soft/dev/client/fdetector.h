
#ifndef __FDETECTOR_H__
#define __FDETECTOR_H__


template<class fp_type,unsigned lg_size>
class CFFT;


// Warning! CSysTicks should be initialized before!
class CFDetector
{
          static const unsigned FFT_LG2 = 7;
          static const unsigned FFT_SAMPLES = 1 << FFT_LG2;
          static const unsigned FFT_SPECTRUM = FFT_SAMPLES/2;
          static const unsigned EXPLOSION_AFFECTED_FREQUENCES = FFT_SPECTRUM*9/10; // 90% of all spectrum

          typedef float fp_type;  // float much faster than double on our CPU!
          typedef fp_type SPECTRUM[FFT_SPECTRUM];

          unsigned m_1ms_samples;  // how much samples in 1 msec
          unsigned m_min_fight_len_ms;
          unsigned m_max_fight_len_ms;
          float m_db_fight_leap;

          typedef struct {
           unsigned ts;
           short *pcm;    // [m_1ms_samples]
          } CHUNK1MS;

          static const unsigned BUFF_CHUNKS = 200;  // in msec, should be more than FFT_SAMPLES/m_1ms_samples

          struct {
           CHUNK1MS ar[BUFF_CHUNKS];   // WARNING!!! check memory usage!
           unsigned rdpos;
           volatile unsigned wrpos;    // changed from IRQ by Push1ms()
          } m_buff;

          SPECTRUM m_prev;
          bool b_firsttime;
          
          struct {
           unsigned ts;     // timestamp of possible detected fight
           unsigned iters;  // length of detected fight in iterations (each iteration is FFT_SAMPLES length)
           float avg_db;    // dB of explosion fight
          } m_fight;

          typedef CFFT<fp_type,FFT_LG2> FFT;
          FFT *p_fft;

          fp_type m_window[FFT_SAMPLES];  // spectrum window, 1/32768 is also included here!

  public:
          CFDetector(int sample_rate,int min_fight_len_ms,int max_fight_len_ms,float db_fight_leap);
          ~CFDetector();
          
          void Push1ms(const short *samples);  // can be safe called from IRQ
          bool PopResult(unsigned& _ts,unsigned& _length_ms,float& _db_amp);

  private:
          fp_type dB(fp_type curr,fp_type base,fp_type min_value);
          fp_type sqr(fp_type x);

};



#endif
