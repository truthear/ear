
#include "include.h"


volatile int CIRQDisable::m_cnt = 0;


CIRQDisable::CIRQDisable()
{
  __disable_irq();
  m_cnt++;
}


CIRQDisable::~CIRQDisable()
{
  m_cnt--;
  if ( m_cnt == 0 )
     {
       __enable_irq();
     }
}

