
#ifndef __TOOLS_H__
#define __TOOLS_H__


#define SAFEDELETE(obj)   { if ( obj ) delete obj; obj = NULL; }

#ifndef ASSERT
#define ASSERT     assert
#endif

#define NNS(str)   ((str)?(str):"")
#define NNSW(str)   ((str)?(str):L"")
#define MAX(a,b)   ((a)>(b)?(a):(b))
#define MIN(a,b)   ((a)<(b)?(a):(b))
#define STRSIZE(_s) (sizeof(_s)/sizeof(_s[0]))


class CCSGuard
{
           CRITICAL_SECTION *p_cs;
  public:
           CCSGuard(const CRITICAL_SECTION &cs)
           {
             p_cs = (CRITICAL_SECTION*)&cs;
             EnterCriticalSection(p_cs);
           }

           ~CCSGuard()
           {
             LeaveCriticalSection(p_cs);
           }
};


class CUnicode
{
           WCHAR *s_out;

  public:
           CUnicode(const char *s,int codepage=CP_THREAD_ACP);
           ~CUnicode();

           operator const WCHAR* () const { return s_out ? s_out : L""; }
           const WCHAR* Text() const { return s_out ? s_out : L""; }
           const WCHAR* RawText() const { return s_out; }
};


class CUTF8Decoder : public CUnicode
{
  public:
           CUTF8Decoder(const char *s) : CUnicode(s,CP_UTF8) {}
};


class CANSI
{
           char *s_out;

  public:
           CANSI(const WCHAR *s,int codepage=CP_THREAD_ACP);
           ~CANSI();

           operator const char* () const { return s_out ? s_out : ""; }
           const char* Text() const { return s_out ? s_out : ""; }
};


class CUTF8Encoder : public CANSI
{
  public:
           CUTF8Encoder(const WCHAR *s) : CANSI(s,CP_UTF8) {}
};


BOOL IsStrEmpty(const char *s);
BOOL IsStrEmpty(const WCHAR *s);
BOOL IsFileExist(const char *s);
char* GetCurrentAppPath(char *s);
char* GetFileNameInTempDir(const char *filename,char *out,BOOL create_dir);
std::string Base64Decode(const char *b64);
std::wstring GetDBFilename();
BOOL CanReadNonBlocked(SOCKET s,unsigned timeout_ms=0);
void FinishSocket(SOCKET& s);



class CLocalDB : public CSQLite
{
  public:
          CLocalDB(BOOL is_read_only=FALSE)
             : CSQLite(::GetDBFilename().c_str(),is_read_only?DB_LOCK_READ_TIMEOUT_MS:DB_LOCK_WRITE_TIMEOUT_MS,is_read_only)
          {
            if ( !is_read_only )
               {
                 TurnOffSyncWrite();  // speedup
               }
          }
};



#endif
