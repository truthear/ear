
#include "include.h"

#include <math.h>
#include <complex>
#include "fft.h"



CFDetector::CFDetector(int sample_rate,unsigned buffer_msec)
{
  assert(MIN_FIGHT_LEN_MS>0);
  assert(MAX_FIGHT_LEN_MS>0);
  assert(MIN_FIGHT_LEN_MS<=MAX_FIGHT_LEN_MS);

  assert(sample_rate>0);
  assert((sample_rate%1000)==0);
  
  m_1ms_samples = sample_rate/1000;

  assert(FFT_SAMPLES>=m_1ms_samples);
  assert((FFT_SAMPLES%m_1ms_samples)==0);

  buffer_msec = MAX(buffer_msec,FFT_SAMPLES/m_1ms_samples+1);

  p_buff = new CBuff(buffer_msec,m_1ms_samples);

  for ( unsigned n = 0; n < FFT_SPECTRUM; n++ )
      {
        m_ambient[n] = 0.0;
      }

  m_stage = STAGE_FIRSTTIME;

  m_fight.ts = 0;
  m_fight.iters = 0;
  m_fight.avg_db = 0;

  p_fft = new FFT;

  for ( unsigned n = 0; n < FFT_SAMPLES; n++ )
      {
        m_window[n] = 0.54 - 0.46 * cos(6.283185307179586476925286766559*n/(FFT_SAMPLES-1));   // Hamming window
        m_window[n] /= 32768.0;
      }
}


CFDetector::~CFDetector()
{
  SAFEDELETE(p_fft);
  SAFEDELETE(p_buff);
}


// should be IRQ safe!
void CFDetector::Push1ms(unsigned ts,const short *samples)
{
  if ( samples )
     {
       p_buff->SetTS(ts);
       p_buff->SetPCM(samples);
       p_buff->IncWrPos();
     }
}


bool CFDetector::PopResult(unsigned& _ts,unsigned& _length_ms,float& _db_amp)
{
  bool rc = false;

  // determine if data ready
  unsigned ms_ready = p_buff->GetReadyToReadCount();
  unsigned ms_needed_min = FFT_SAMPLES/m_1ms_samples;

  if ( ms_ready >= ms_needed_min )
     {
       // save ts
       unsigned ts = p_buff->GetTS(); 

       // fill samples ar
       FFT::cplx_type cpar[FFT_SAMPLES];
       unsigned cpar_idx = 0;
       for ( unsigned n = 0; n < ms_needed_min; n++ )
           {
             const short *src = p_buff->GetPCM();
             for ( unsigned m = 0; m < m_1ms_samples; m++ )
                 {
                   cpar[cpar_idx] = FFT::cplx_type((fp_type)src[m]*m_window[cpar_idx]);
                   cpar_idx++;
                 }

             p_buff->IncRdPos();
           }

       // make spectrum
       p_fft->Perform(cpar,false);

       SPECTRUM spc;
       for ( unsigned n = 0; n < FFT_SPECTRUM; n++ )
           {
             spc[n] = std::norm(cpar[n]);  // sqr value, faster than std::abs()
           }

       // analyze
       if ( m_stage == STAGE_FIRSTTIME )
          {
            m_stage = STAGE_AMBIENT;
            m_ambient = spc;
          }
       else
          {
            // calc dB of current data
            fp_type db_avg = 0;
            fp_type min_spc_sqr_value = sqr((fp_type)(FFT_SPECTRUM*0.0001));
            unsigned num_positives = 0;
            for ( unsigned n = 0; n < FFT_SPECTRUM; n++ )
                {
                  fp_type db = (fp_type)0.5*dB(spc[n],m_ambient[n],min_spc_sqr_value); // 0.5 because of we use norm(), but not abs()
                  db_avg += db;
                  num_positives += (db >= (fp_type)0 ? 1 : 0);
                }
            db_avg /= (fp_type)FFT_SPECTRUM;

            bool is_fight_detected = (num_positives >= FIGHT_AFFECTED_FREQUENCES && db_avg > DB_FIGHT_LEAP);
            bool is_indefinite_detected = (!is_fight_detected && db_avg > DB_INDEFINITE_LEAP);

            bool add_fight = false;     // is need to add fight data?
            bool finish_fight = false;  // is fight finished
            
            if ( m_stage == STAGE_AMBIENT )
               {
                 if ( is_indefinite_detected )
                    {
                      m_stage = STAGE_INDEFINITE;
                    }
                 else
                 if ( is_fight_detected )
                    {
                      m_stage = STAGE_FIGHT;
                      add_fight = true;
                    }
                 else
                    {
                      m_ambient = spc;
                    }
               }
            else
            if ( m_stage == STAGE_INDEFINITE )
               {
                 if ( is_fight_detected )
                    {
                      m_stage = STAGE_FIGHT;
                      add_fight = true;
                    }
                 else
                    {
                      m_stage = STAGE_AMBIENT;
                      m_ambient = spc;
                    }
               }
            else
            if ( m_stage == STAGE_FIGHT )
               {
                 if ( is_fight_detected )
                    {
                      add_fight = true;
                    }
                 else
                    {
                      m_stage = STAGE_AMBIENT;
                      m_ambient = spc;
                      finish_fight = true;
                    }
               }
            
            if ( add_fight )
               {
                 if ( m_fight.iters == 0 )  // first time?
                    {
                      m_fight.ts = ts;
                    }

                 m_fight.iters++;
                 m_fight.avg_db += db_avg;
               }

            unsigned length_ms = m_fight.iters * (FFT_SAMPLES/m_1ms_samples);

            if ( length_ms >= MAX_FIGHT_LEN_MS )
               {
                 finish_fight = true;
               }

            if ( finish_fight )
               {
                 if ( length_ms >= MIN_FIGHT_LEN_MS )
                    {
                      // got result!
                      assert(m_fight.iters>0);
                      _ts = m_fight.ts;
                      _length_ms = length_ms;
                      _db_amp = m_fight.avg_db/m_fight.iters;
                      
                      rc = true;
                    }

                 // reset
                 m_fight.ts = 0;
                 m_fight.iters = 0;
                 m_fight.avg_db = 0;

                 m_stage = STAGE_AMBIENT;
                 m_ambient = spc;
               }
          }
     }

  return rc;
}


CFDetector::fp_type CFDetector::dB(fp_type curr,fp_type base,fp_type min_value)
{
  if ( curr < min_value && base < min_value )
     {
       return (fp_type)0.0;
     }
  else
     {
       curr = MAX(curr,min_value);
       base = MAX(base,min_value);

       fp_type amp = curr/base;

       amp = (sizeof(fp_type)==sizeof(float) ? log10f(amp) : log10(amp));
       
       amp *= (fp_type)20.0;

       return amp;
     }
}





