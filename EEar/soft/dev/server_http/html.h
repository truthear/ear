
#ifndef __HTML_H__
#define __HTML_H__



class COutput
{
          TMODINTF *p_itf;

  public:
          COutput(TMODINTF *i,const WCHAR *title);
          ~COutput();

          void operator += (const char *s);
          void operator += (const WCHAR *s);
          void operator += (const std::string& s);
          void operator += (const std::wstring& s);

  private:
          void HTMLWrite(const char *s);
};



std::string HTMLFilter(const std::string& s);
std::wstring HTMLFilter(const std::wstring& s);
std::string HTMLFilter(const char *s);
std::wstring HTMLFilter(const WCHAR *s);



#endif
