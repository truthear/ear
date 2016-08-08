
#ifndef __TOOLS_H__
#define __TOOLS_H__


#define MAX(a,b)   ((a)>(b)?(a):(b))
#define MIN(a,b)   ((a)<(b)?(a):(b))

#define CLEAROBJ(o)   memset(&(o),0,sizeof(o))


typedef signed long long OURTIME;   // time_t*1000+msec, must be signed (because of arithmetic operations)!


// WARNING!!! works only on single-core CPU!
// should be created in main thread only!
class CCriticalSection
{
          volatile int m_lock;
  
  public:
          CCriticalSection() : m_lock(0) {}
          
          // can be called from IRQ:
          bool IsUnlocked() const { return m_lock == 0; }

  protected:
          friend class CCSGuard;

          void Lock() { m_lock++; /*not atomic? is ok!*/ }
          void Unlock() { m_lock--; /*not atomic? is ok!*/ }
};


// WARNING!!! works only on single-core CPU!
// should be used in main thread only!
class CCSGuard
{
          CCriticalSection& o_cs;

  public:
          CCSGuard(CCriticalSection& _cs) : o_cs(_cs) { o_cs.Lock(); }
          ~CCSGuard() { o_cs.Unlock(); }
};



OURTIME ConvertOurTime(int yyyy,int mm,int dd,int hh,int nn,int ss,int msec);
char* OurTimeToString(OURTIME t,char *s); // WARNING!!! not IRQhandler/thread safe! problem when simulatenous call in main and IRQ



#endif
