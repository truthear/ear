
#include "include.h"



CGoogleMap::CGoogleMap(TMODINTF *i,const WCHAR *title)
{
  p_out = new CHTML(i,title,"onload=\"Init()\"");
  p_out->AddRawString(LoadRawResource(g_instance,IDR_MAPBEGIN));
  is_first = TRUE;
}


CGoogleMap::~CGoogleMap()
{
  p_out->AddRawString(LoadRawResource(g_instance,IDR_MAPEND));
  SAFEDELETE(p_out);
}


void CGoogleMap::Add(double lat,double lon,const std::string& label)
{
  std::string s = label;
  s = ReplaceSymbol(s,'\"','\'');
  s = ReplaceSymbol(s,'\\','/');
  s = ReplaceSymbol(s,'\r',' ');
  s = ReplaceSymbol(s,'\n',' ');
  s = ReplaceSymbol(s,'\t',' ');

  (*p_out) += CFormat("%c[%.7f,%.7f,\"%s\"]\n",is_first?' ':',',lat,lon,s.c_str());

  is_first = FALSE;
}


//////////////////////////



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


