
#ifndef __TOOLS_H__
#define __TOOLS_H__


#define MAX(a,b)   ((a)>(b)?(a):(b))
#define MIN(a,b)   ((a)<(b)?(a):(b))

#define CLEAROBJ(o)   memset(&(o),0,sizeof(o))

#define NNS(str)   ((str)?(str):"")

#define SAFEDELETE(obj)   { if ( obj ) delete obj; obj = NULL; }


typedef signed long long OURTIME;   // number of msec since 2000-01-01, must be signed (because of arithmetic operations)!


class COurTime
{
          static const OURTIME MAX_TIME_VALUE = 3155759999999;
          
          OURTIME m_time;

  public:
          COurTime() : m_time(-1) {}
          COurTime(OURTIME t) : m_time(t) {}
          COurTime(int yy,int mm,int dd,int hh,int nn,int ss,int msec);

          operator OURTIME () const { return m_time; }
          OURTIME GetValue() const { return m_time; }

          bool Unpack(int& yy,int& mm,int& dd,int& hh,int& nn,int& ss,int& msec) const;
          char* ToString(char *s) const;
          std::string AsString() const;

  private:
          static bool IsLeapYear(int yy);
          static const int* GetMonthTable(int yy);

};


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


template<int _size>
class CFormatStr
{
          char m_buffer[_size];
  public:
          CFormatStr(const char *format,...)
          {
            m_buffer[0] = 0;
            va_list args;
            va_start(args,format);
            vsprintf(m_buffer,format,args);
            va_end(args);
          }

          operator const char* () const { return m_buffer; }
};


typedef CFormatStr<256> CFormat;


bool IsStrEmpty(const char *s);




#endif
