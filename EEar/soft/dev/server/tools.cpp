
#include "include.h"




BOOL IsStrEmpty(const char *s)
{
  return !s || !s[0];
}


BOOL IsStrEmpty(const WCHAR *s)
{
  return !s || !s[0];
}


BOOL IsFileExist(const char *s)
{
  return (GetFileAttributes(s) != INVALID_FILE_ATTRIBUTES);
}


char* GetCurrentAppPath(char *s)
{
  if ( s )
     {
       s[0] = 0;
       GetModuleFileName(NULL,s,MAX_PATH);
       PathRemoveFileSpec(s);
     }

  return s;
}


char* GetFileNameInTempDir(const char *filename,char *out,BOOL create_dir)
{
  if ( out )
     {
       char s[MAX_PATH] = "";
       GetTempPath(sizeof(s),s);

       if ( !IsStrEmpty(s) )
          {
            if ( create_dir )
               CreateDirectory(s,NULL);
            
            PathAppend(s,filename);
          }

       lstrcpy(out,s);
     }

  return out;
}



CUnicode::CUnicode(const char *s,int codepage)
{
  if ( s )
     {
       int alloc_chars = lstrlen(s)+1;
       
       s_out = (WCHAR*)malloc(alloc_chars*sizeof(WCHAR));
       s_out[0] = 0;

       MultiByteToWideChar(codepage,0,s,-1,s_out,alloc_chars);
     }
  else
     {
       s_out = NULL;
     }
}


CUnicode::~CUnicode()
{
  if ( s_out )
     {
       free(s_out);
       s_out = NULL;
     }
}



CANSI::CANSI(const WCHAR *s,int codepage)
{
  if ( s )
     {
       int alloc_chars = (lstrlenW(s)+1)*4;
       
       s_out = (char*)malloc(alloc_chars*sizeof(char));
       s_out[0] = 0;

       WideCharToMultiByte(codepage,0,s,-1,s_out,alloc_chars,NULL,NULL);
     }
  else
     {
       s_out = NULL;
     }
}


CANSI::~CANSI()
{
  if ( s_out )
     {
       free(s_out);
       s_out = NULL;
     }
}


std::string Base64Decode(const char *b64)
{
  std::string rc;

  if ( !IsStrEmpty(b64) )
     {
       DWORD needed = 0;
       if ( CryptStringToBinary(b64,0,CRYPT_STRING_BASE64,NULL,&needed,NULL,NULL) && needed > 0 )
          {
            BYTE *bin = (BYTE*)malloc(needed);
            if ( bin )
               {
                 if ( CryptStringToBinary(b64,0,CRYPT_STRING_BASE64,bin,&needed,NULL,NULL) )
                    {
                      rc.assign((char*)bin,needed);
                    }

                 free(bin);
               }
          }
     }

  return rc;
}


std::wstring GetDBFilename()
{
  char s[MAX_PATH] = "";
  GetCurrentAppPath(s);
  PathAppend(s,OUR_DB_FILENAME);
  
  return (const WCHAR*)CUnicode(s);
}


BOOL CanReadNonBlocked(SOCKET s,unsigned timeout_ms)
{
  timeval tv;
  tv.tv_sec = timeout_ms/1000;
  tv.tv_usec = (timeout_ms%1000)*1000;

  fd_set rset;
  rset.fd_count = 1;
  rset.fd_array[0] = s;

  return (select(0,&rset,NULL,NULL,&tv) == 1);
}


void FinishSocket(SOCKET& s)
{
  if ( s != INVALID_SOCKET )
     {
       shutdown(s,SD_BOTH);
       closesocket(s);
       s = INVALID_SOCKET;
     }
}


int GetTimeZoneOffsetMinutes()
{
  TIME_ZONE_INFORMATION i;
  ZeroMemory(&i,sizeof(i));
  GetTimeZoneInformation(&i);
  return -i.Bias;
}


const void* LoadRawResource(HINSTANCE our_instance,int id,unsigned& _size)
{
  HRSRC res = FindResource(our_instance,MAKEINTRESOURCE(id),RT_RCDATA);
  if ( res )
     {
       HGLOBAL g = LoadResource(our_instance,res);
       if ( g )
          {
            _size = SizeofResource(our_instance,res);
            return LockResource(g);
          }
     }

  _size = 0;
  return NULL;
}


std::string LoadRawResource(HINSTANCE our_instance,int id)
{
  std::string rc;

  unsigned size = 0;
  const void *p = LoadRawResource(our_instance,id,size);

  if ( p && size > 0 )
     {
       rc.assign((const char*)p,size);
     }

  return rc;
}


std::wstring ReplaceSymbol(const std::wstring& s,WCHAR from,WCHAR to)
{
  std::wstring rc;

  for ( unsigned n = 0; n < s.size(); n++ )
      {
        WCHAR c = s[n];
        rc += (c == from ? to : c);
      }

  return rc;
}


std::string ReplaceSymbol(const std::string& s,char from,char to)
{
  std::string rc;

  for ( unsigned n = 0; n < s.size(); n++ )
      {
        char c = s[n];
        rc += (c == from ? to : c);
      }

  return rc;
}




