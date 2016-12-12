
#ifndef __FDETECTOR_H__
#define __FDETECTOR_H__


template<class fp_type,unsigned lg_size>
class CFFT;


class CFDetector
{
  public:
          static const unsigned VERSION = 0x0100; // recommended to increment in case constants or algorithm changed

  private:
          static const unsigned FFT_LG2 = 7;
          static const unsigned FFT_SAMPLES = 1 << FFT_LG2;
          static const unsigned FFT_SPECTRUM = FFT_SAMPLES/2;

          // fight detection constants:
          static const unsigned MIN_FIGHT_LEN_MS = 40;
          static const unsigned MAX_FIGHT_LEN_MS = 2000;
          static const int DB_FIGHT_LEAP = 9;         // dB leap of fight detection
          static const int DB_INDEFINITE_LEAP = 5;    // dB leap of indefinite stage detection
          static const unsigned FIGHT_AFFECTED_FREQUENCES = FFT_SPECTRUM*90/100; // 90% of all spectrum


          typedef float fp_type;  // float much faster than double on our 32-bit CPU!

          template<class fp_type,unsigned size>
          class CArray
          {
                    fp_type ar[size];
            public:
                    const CArray& operator = (const CArray& other)
                    {
                      for ( unsigned n = 0; n < size; n++ )
                          {
                            ar[n] = other.ar[n];
                          }

                      return *this;
                    }

                    fp_type& operator [] (unsigned idx)
                    {
                      return ar[idx];
                    }

                    const fp_type& operator [] (unsigned idx) const
                    {
                      return ar[idx];
                    }
          };
          
          typedef CArray<fp_type,FFT_SPECTRUM> SPECTRUM;

          unsigned m_1ms_samples;  // how much samples in 1 msec


          class CBuff 
          {
                    unsigned numchunks;
                    unsigned m_chunk_samples;
                    short *p_pcm;    // [numchunks*m_chunk_samples]
                    unsigned *p_ts;  // [numchunks]
                    unsigned rdpos;  
                    volatile unsigned wrpos; // can be changed from IRQ

            public:
                    CBuff(unsigned _numchunks,unsigned _chunk_samples)
                    {
                      numchunks = _numchunks;
                      m_chunk_samples = _chunk_samples;
                      p_pcm = (short*)calloc(numchunks*m_chunk_samples,sizeof(*p_pcm));  // zero clears
                      p_ts = (unsigned*)calloc(numchunks,sizeof(*p_ts));               // zero clears
                      rdpos = 0;
                      wrpos = 0;
                    }

                    ~CBuff()
                    {
                      free(p_ts);
                      free(p_pcm);
                    }

                    unsigned GetReadyToReadCount() const
                    {
                      volatile unsigned widx = wrpos;  // volatile!
                      unsigned ridx = rdpos;

                      if ( widx < ridx )
                         {
                           widx += numchunks;
                         }

                      return widx - ridx;
                    }
             
                    unsigned GetTS() const
                    {
                      return p_ts[rdpos];
                    }

                    const short* GetPCM() const
                    {
                      return p_pcm+rdpos*m_chunk_samples;
                    }

                    void SetTS(unsigned ts)
                    {
                      p_ts[wrpos] = ts;
                    }

                    void SetPCM(const short *samples)
                    {
                      short *dst = p_pcm+wrpos*m_chunk_samples;
                      for ( unsigned n = 0; n < m_chunk_samples; n++ )
                          {
                            *dst++ = *samples++;
                          }
                    }

                    void IncRdPos()
                    {
                      volatile unsigned idx = rdpos;

                      idx++;
                      idx = ((idx == numchunks) ? 0 : idx);

                      rdpos = idx;
                    }

                    void IncWrPos()
                    {
                      volatile unsigned idx = wrpos;

                      idx++;
                      idx = ((idx == numchunks) ? 0 : idx);

                      wrpos = idx;
                    }
          };
          
          CBuff *p_buff;

          SPECTRUM m_ambient;  // previous state, state for compare

          enum {
           STAGE_FIRSTTIME,    // first time call
           STAGE_AMBIENT,      // ambient noise
           STAGE_INDEFINITE,   // before fight
           STAGE_FIGHT,        // in-fight
          } m_stage;
          
          struct {
           unsigned ts;     // timestamp of possible detected fight
           unsigned iters;  // length of detected fight in iterations (each iteration is FFT_SAMPLES length)
           float avg_db;    // dB of explosion fight (average)
          } m_fight;

          typedef CFFT<fp_type,FFT_LG2> FFT;
          FFT *p_fft;

          fp_type m_window[FFT_SAMPLES];  // spectrum window, 1/32768 is also included here!

  public:
          CFDetector(int sample_rate,unsigned buffer_msec);
          ~CFDetector();
          
          void Push1ms(unsigned ts,const short *samples);  // can be safe called from IRQ
          bool PopResult(unsigned& _ts,unsigned& _length_ms,float& _db_amp);

  private:
          static fp_type dB(fp_type curr,fp_type base,fp_type min_value);
          static fp_type sqr(fp_type x) { return x*x; }

};



#endif
