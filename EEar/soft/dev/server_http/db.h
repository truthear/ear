
#ifndef __DB_H__
#define __DB_H__


class CLocalDB;
class CSQLiteQuery;


class CReadDBTable
{
          CLocalDB *p_db;
          CSQLiteQuery *p_q;

  public:
          CReadDBTable(const WCHAR *query);
          ~CReadDBTable();

          BOOL FetchRow();

          int GetAsInt(int col);
          __int64 GetAsInt64(int col);
          double GetAsDouble(int col);
          std::wstring GetAsText(int col);
};




#endif
