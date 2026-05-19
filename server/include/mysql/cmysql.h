#pragma once

#include <mysql.h>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMutex>
#include <cstring>

class CMySql
{
public:
    CMySql(void);
    ~CMySql(void);

    bool ConnectMySql(char *host, char *user, char *pass, char *db, short nport = 3306);
    void DisConnect();
    bool SelectMySql(const char* szSql, int nColumn, QStringList& lstStr);
    bool GetTables(const char* szSql, QStringList& lstStr);
    bool UpdateMySql(const char* szSql);

    bool PrepareStmt(const char* sql);
    bool BindParamInt(int paramIndex, int value);
    bool BindParamString(int paramIndex, const char* value);
    bool BindParamLongString(int paramIndex, const char* value, unsigned long length);
    bool ExecuteStmt();
    bool StoreResult();
    bool FetchRow();
    int GetColumnInt(int columnIndex);
    QByteArray GetColumnString(int columnIndex);
    void CloseStmt();
    bool EscapeString(const char* src, char* dest, unsigned long length);

private:
    MYSQL *m_sock;
    MYSQL_STMT *m_stmt;
    QVector<MYSQL_BIND> m_bindParams;
    QVector<MYSQL_BIND> m_bindResults;
    QVector<char> m_stringBuffer;
    QVector<unsigned long> m_paramLengths;
    QVector<int> m_intBuffer;
    bool m_stmtActive;

    MYSQL_RES *m_results;
    MYSQL_ROW m_record;

    static QMutex m_mysqlMutex;
};
