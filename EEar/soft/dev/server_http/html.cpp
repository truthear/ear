
#include "include.h"




CHTML::CHTML(TMODINTF *i,const WCHAR *title,const char *body_parms)
{
  p_itf = i;

  p_itf->Header("Expires","Thu, 19 Nov 1981 08:52:00 GMT",TRUE);
  p_itf->Header("Cache-Control","no-store, no-cache, must-revalidate, post-check=0, pre-check=0",TRUE);
  p_itf->Header("Pragma","no-cache",TRUE);

  p_itf->Echo("<!DOCTYPE html>\n");
  p_itf->Echo("<html>\n");
  p_itf->Echo("<head>\n");
  p_itf->Echo("<meta charset=\"utf-8\">\n");
  p_itf->Echo(CFormat("<title>%s</title>\n",(const char*)CUTF8Encoder(HTMLFilter(title).c_str())));
  p_itf->Echo("<link rel=\"icon\" href=\"favicon.ico\" type=\"image/x-icon\" />\n");
  p_itf->Echo("<link rel=\"shortcut icon\" href=\"favicon.ico\" type=\"image/x-icon\" />\n");
  p_itf->Echo("</head>\n");
  p_itf->Echo(IsStrEmpty(body_parms)?"<body>\n":CFormat("<body %s>\n",body_parms));
}


CHTML::~CHTML()
{
  p_itf->Echo("</body>\n");
  p_itf->Echo("</html>\n");
}


void CHTML::AddRawString(const char *s)
{
  p_itf->Echo(NNS(s));
}


void CHTML::AddRawString(const std::string& s)
{
  AddRawString(s.c_str());
}


void CHTML::operator += (const char *s)
{
  p_itf->Echo((const char*)CUTF8Encoder((const WCHAR*)CUnicode(s)));
}


void CHTML::operator += (const WCHAR *s)
{
  p_itf->Echo((const char*)CUTF8Encoder(s));
}


void CHTML::operator += (const std::string& s)
{
  (*this) += s.c_str();
}


void CHTML::operator += (const std::wstring& s)
{
  (*this) += s.c_str();
}


void CHTML::operator += (int v)
{
  (*this) += CFormat("%d",v);
}


void CHTML::operator += (__int64 v)
{
  (*this) += CFormat("%I64d",v);
}


void CHTML::operator += (double v)
{
  (*this) += CFormat("%.7f",v);
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


std::string HTMLFilter(const char *s)
{
  return HTMLFilter(std::string(NNS(s)));
}


std::wstring HTMLFilter(const WCHAR *s)
{
  return HTMLFilter(std::wstring(NNSW(s)));
}


