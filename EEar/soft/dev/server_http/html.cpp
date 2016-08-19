
#include "include.h"




COutput::COutput(TMODINTF *i,const WCHAR *title)
{
  p_itf = i;

  p_itf->Header("Expires","Thu, 19 Nov 1981 08:52:00 GMT",TRUE);
  p_itf->Header("Cache-Control","no-store, no-cache, must-revalidate, post-check=0, pre-check=0",TRUE);
  p_itf->Header("Pragma","no-cache",TRUE);

  p_itf->Echo("<!DOCTYPE html>\n");
  p_itf->Echo("<html>\n");
  p_itf->Echo("<head>\n");
  p_itf->Echo("<meta charset=\"utf-8\">\n");
  p_itf->Echo(CFormat("<title>%s</title>\n",(const char*)CUTF8Encoder(title)));
  p_itf->Echo("<link rel=\"icon\" href=\"favicon.ico\" type=\"image/x-icon\" />\n");
  p_itf->Echo("<link rel=\"shortcut icon\" href=\"favicon.ico\" type=\"image/x-icon\" />\n");
  p_itf->Echo("</head>\n");
  p_itf->Echo("<body>\n");
}


COutput::~COutput()
{
  p_itf->Echo("</body>\n");
  p_itf->Echo("</html>\n");
}


void COutput::operator += (const char *s)
{
  p_itf->Echo((const char*)CUTF8Encoder((const WCHAR*)CUnicode(s)));
}


void COutput::operator += (const WCHAR *s)
{
  p_itf->Echo((const char*)CUTF8Encoder(s));
}


void COutput::operator += (const std::string& s)
{
  (*this) += s.c_str();
}


void COutput::operator += (const std::wstring& s)
{
  (*this) += s.c_str();
}


/////////////////


std::string HTMLFilter(const std::string& s)
{
  std::string rc;
  
  for ( unsigned n = 0; n < s.size(); n++ )
      {
        char c = s[n];

        if ( c == '<' )
         rc += "&lt;";
        else
        if ( c == '>' )
         rc += "&gt;";
        else
        if ( c == '&' )
         rc += "&amp;";
        else
        if ( c == '\n' )
         rc += "<br>";
        else
        if ( c != '\r' )
         rc += c;
      }

  return rc;
}


std::wstring HTMLFilter(const std::wstring& s)
{
  std::wstring rc;
  
  for ( unsigned n = 0; n < s.size(); n++ )
      {
        WCHAR c = s[n];

        if ( c == '<' )
         rc += L"&lt;";
        else
        if ( c == '>' )
         rc += L"&gt;";
        else
        if ( c == '&' )
         rc += L"&amp;";
        else
        if ( c == '\n' )
         rc += L"<br>";
        else
        if ( c != '\r' )
         rc += c;
      }

  return rc;
}


