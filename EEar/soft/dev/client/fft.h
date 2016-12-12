
#ifndef __FFT_H__
#define __FFT_H__



template<class fp_type,unsigned lg_size>
class CFFT
{
  public:
          typedef std::complex<fp_type> cplx_type;

  private:
          static const unsigned ar_size = 1 << lg_size;

          std::vector<std::pair<unsigned,unsigned>> m_swaps;
          std::vector<cplx_type> m_w[lg_size];
          std::vector<cplx_type> m_w_invert[lg_size];
  
  public:
          CFFT()
          {
            FillSwapsTable(m_swaps);
            FillWTable(m_w,false);
            FillWTable(m_w_invert,true);
          }

          void Perform(cplx_type *ar,bool invert) const
          {
            for ( unsigned n = 0; n < m_swaps.size(); n++ )
                {
                  std::swap<cplx_type>(ar[m_swaps[n].first],ar[m_swaps[n].second]);
                }

            unsigned idx = 0;
            for ( unsigned len = 2; len <= ar_size; len *= 2 ) 
                {
                  const std::vector<cplx_type>& w_ar = invert ? m_w_invert[idx] : m_w[idx];
                  idx++;

                  for ( unsigned i = 0; i < ar_size; i += len ) 
                      {
                        for ( unsigned j = 0; j < len/2; j++ )
                            {
                              cplx_type u = ar[i+j];
                              cplx_type v = ar[i+j+len/2] * w_ar[j];
                              ar[i+j] = u + v;
                              ar[i+j+len/2] = u - v;
                            }
                      }
                }

            if ( invert )
               {
                 for ( unsigned n = 0; n < ar_size; n++ )
                     {
                       ar[n] /= ar_size;
                     }
               }
          }

  private:
          static void FillSwapsTable(std::vector<std::pair<unsigned,unsigned>>& swaps)
          {
            for ( unsigned n = 0; n < ar_size; n++ )
                {
                  if ( n < RevBits(n) )
                     {
                       swaps.push_back(std::pair<unsigned,unsigned>(n,RevBits(n)));
                     }
                }
          }

          static void FillWTable(std::vector<cplx_type> *t,bool invert)
          {
            unsigned idx = 0;
            for ( unsigned len = 2; len <= ar_size; len *= 2 ) 
                {
                  double ang = (double)6.283185307179586476925286766559/len;
                  ang = invert ? -ang : ang;
                  std::complex<double> wlen(cos(ang),sin(ang));

                  std::vector<cplx_type>& ar = t[idx++];
                  ar.reserve(len/2);  // optimization

                  std::complex<double> w(1);
                  for ( unsigned j = 0; j < len/2; j++ )
                      {
                        ar.push_back(cplx_type(w.real(),w.imag()));
                        w *= wlen;
                      }
                }   
          }
          
          static unsigned RevBits(unsigned idx)
          {
            unsigned res = 0;

            for ( unsigned n = 0; n < lg_size; n++ )
                {
                  if ( idx & (1<<n) )
                     {
                       res |= 1 << (lg_size-1-n);
                     }
                }

            return res;
          }
};



#endif
