
#ifndef __HTML_H__
#define __HTML_H__



class CHTML
{
          TMODINTF *p_itf;

  public:
          CHTML(TMODINTF *i,const WCHAR *title,const char *body_parms=NULL);
          ~CHTML();

          void AddRawString(const char *s); // no UTF8 conversion here!
          void AddRawString(const std::string& s); // no UTF8 conversion here!

          void operator += (const char *s);
          void operator += (const WCHAR *s);
          void operator += (const std::string& s);
          void operator += (const std::wstring& s);
          void operator += (int v);
          void operator += (__int64 v);
          void operator += (double v);
};

class CPre
{
          TMODINTF *p_itf;
  public:
          CPre(TMODINTF *i)
          {
            p_itf = i;
            p_itf->Echo("<pre>\n");
          }
          ~CPre()
          {
            p_itf->Echo("</pre>\n");
          }
};

class CTable
{
          TMODINTF *p_itf;
  public:
          CTable(TMODINTF *i,const char *parms="border=\"1\" cellspacing=\"0\" cellpadding=\"3\"")
          {
            p_itf = i;
            p_itf->Echo(IsStrEmpty(parms)?"<table>\n":CFormat("<table %s>\n",parms));
          }
          ~CTable()
          {
            p_itf->Echo("</table>\n");
          }
};

class CTableRow
{
          TMODINTF *p_itf;
  public:
          CTableRow(TMODINTF *i)
          {
            p_itf = i;
            p_itf->Echo("<tr>\n");
          }
          ~CTableRow()
          {
            p_itf->Echo("</tr>\n");
          }
};

class CTableCell
{
          TMODINTF *p_itf;
  public:
          CTableCell(TMODINTF *i,const char *parms=NULL)
          {
            p_itf = i;
            p_itf->Echo(IsStrEmpty(parms)?"<td>":CFormat("<td %s>",parms));
          }
          ~CTableCell()
          {
            p_itf->Echo("</td>\n");
          }
};

class CTableCellHeader : public CTableCell
{
  public:
          CTableCellHeader(TMODINTF *i) : CTableCell(i,"align=\"center\" style=\"font-weight:bold;\"") {}
};

class CBold
{
          TMODINTF *p_itf;
  public:
          CBold(TMODINTF *i)
          {
            p_itf = i;
            p_itf->Echo("<b>");
          }
          ~CBold()
          {
            p_itf->Echo("</b>");
          }
};

class CUnderline
{
          TMODINTF *p_itf;
  public:
          CUnderline(TMODINTF *i)
          {
            p_itf = i;
            p_itf->Echo("<u>");
          }
          ~CUnderline()
          {
            p_itf->Echo("</u>");
          }
};

class CAnchor
{
          TMODINTF *p_itf;
  public:
          CAnchor(TMODINTF *i,const char *href,const char *target="_blank")
          {
            p_itf = i;
            p_itf->Echo(CFormat("<a href=\"%s\" target=\"%s\">",NNS(href),NNS(target)));
          }
          ~CAnchor()
          {
            p_itf->Echo("</a>");
          }
};


class CHTMLTools
{
  public:
          static std::string Filter(const std::string& s);
          static std::wstring Filter(const std::wstring& s);
          static std::string Filter(const char *s);
          static std::wstring Filter(const WCHAR *s);
          static void ProduceHeaderCell(TMODINTF *i,CHTML& out,const char *text,BOOL mark=FALSE);
          static void ProduceGeoCell(TMODINTF *i,CHTML& out,CReadDBTable& db,int lat,int lon);
          static void ProduceVerCell(TMODINTF *i,CHTML& out,CReadDBTable& db,int ver1,int ver2);
          static void ProduceIntCell(TMODINTF *i,CHTML& out,CReadDBTable& db,int col);
          static void ProduceDoubleCell(TMODINTF *i,CHTML& out,CReadDBTable& db,int col,int digits);
          static void ProduceTextCell(TMODINTF *i,CHTML& out,CReadDBTable& db,int col);
          static void ProduceTimeCell(TMODINTF *i,CHTML& out,CReadDBTable& db,int col,const char *postfix=NULL);

};




#endif
