
#include "include.h"



CReadDBTable::CReadDBTable(const WCHAR *query)
{
  p_db = new CLocalDB(TRUE);
  p_q = p_db->CreateQuery(query);
}


CReadDBTable::~CReadDBTable()
{
  p_q->Destroy();
  SAFEDELETE(p_db);
}


BOOL CReadDBTable::FetchRow()
{
  BOOL is_data = FALSE;
  return p_q->Step(&is_data) && is_data;
}


int CReadDBTable::GetAsInt(int col)
{
  return p_q->GetAsInt(col);
}


__int64 CReadDBTable::GetAsInt64(int col)
{
  return p_q->GetAsInt64(col);
}


double CReadDBTable::GetAsDouble(int col)
{
  return p_q->GetAsDouble(col);
}


std::wstring CReadDBTable::GetAsText(int col)
{
  return p_q->GetAsText(col);
}
