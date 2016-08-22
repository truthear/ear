
#ifndef __UTILS_H__
#define __UTILS_H__


class CHTML;

class CGoogleMap
{
          CHTML *p_out;
          BOOL is_first;
  public:
          CGoogleMap(TMODINTF *i,const WCHAR *title);
          ~CGoogleMap();
          void Add(double lat,double lon,const std::string& label);
};


const void* LoadRawResource(HINSTANCE our_instance,int id,unsigned& _size);
std::string LoadRawResource(HINSTANCE our_instance,int id);
std::wstring ReplaceSymbol(const std::wstring& s,WCHAR from,WCHAR to);
std::string ReplaceSymbol(const std::string& s,char from,char to);



#endif
