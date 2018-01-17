
#ifndef __TOOLS_H__
#define __TOOLS_H__


#define MAX(a,b)   ((a)>(b)?(a):(b))
#define MIN(a,b)   ((a)<(b)?(a):(b))

#define CLEAROBJ(o)   memset(&(o),0,sizeof(o))

#define NNS(str)   ((str)?(str):"")

#define SAFEDELETE(obj)   { if ( obj ) delete obj; obj = NULL; }



class CIRQDisable
{
          static volatile int m_cnt;
  public:
          CIRQDisable();
          ~CIRQDisable();
};



#endif
