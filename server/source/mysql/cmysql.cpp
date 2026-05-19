#include "cmysql.h"
#include <QDebug>

QMutex CMySql::m_mysqlMutex;

CMySql::CMySql(void) : m_stmt(nullptr), m_stmtActive(false), m_results(nullptr), m_record(nullptr) {
    m_sock = new MYSQL;
    mysql_init(m_sock);
    mysql_set_character_set(m_sock, "utf8mb4");
}

CMySql::~CMySql(void) {
    CloseStmt();
    DisConnect();
    if (m_sock) {
        delete m_sock;
        m_sock = nullptr;
    }
}

void CMySql::DisConnect() {
    mysql_close(m_sock);
}

bool CMySql::ConnectMySql(char *host, char *user, char *pass, char *db, short nport) {
    if (!mysql_real_connect(m_sock, host, user, pass, db, nport, nullptr, CLIENT_MULTI_STATEMENTS)) {
        qDebug() << "连接数据库失败:" << mysql_error(m_sock);
        return false;
    }

    mysql_set_character_set(m_sock, "utf8mb4");
    mysql_query(m_sock, "SET NAMES utf8mb4");

    return true;
}

bool CMySql::GetTables(const char* szSql, QStringList& lstStr) {
    if (mysql_query(m_sock, szSql)) {
        return false;
    }

    m_results = mysql_store_result(m_sock);
    if (nullptr == m_results) {
        return false;
    }

    while ((m_record = mysql_fetch_row(m_results))) {
        lstStr.append(m_record[0]);
    }
    mysql_free_result(m_results);
    return true;
}

bool CMySql::SelectMySql(const char* szSql, int nColumn, QStringList& lstStr) {
    QMutexLocker locker(&m_mysqlMutex);

    if (mysql_query(m_sock, szSql)) {
        qDebug() << "查询数据库失败:" << mysql_error(m_sock);
        return false;
    }

    m_results = mysql_store_result(m_sock);
    if (nullptr == m_results) {
        qDebug() << "查询结果为空";
        return false;
    }

    while ((m_record = mysql_fetch_row(m_results))) {
        for (int i = 0; i < nColumn; i++) {
            if (!m_record[i]) {
                lstStr.append("");
            } else {
                lstStr.append(m_record[i]);
            }
        }
    }
    mysql_free_result(m_results);
    return true;
}

bool CMySql::UpdateMySql(const char* szSql) {
    if (!szSql) {
        return false;
    }

    QMutexLocker locker(&m_mysqlMutex);

    if (mysql_query(m_sock, szSql)) {
        qDebug() << "UpdateMySql failed:" << mysql_error(m_sock);
        qDebug() << "SQL:" << szSql;
        return false;
    }
    return true;
}

bool CMySql::PrepareStmt(const char* sql) {
    QMutexLocker locker(&m_mysqlMutex);

    CloseStmt();
    m_stmt = mysql_stmt_init(m_sock);
    if (!m_stmt) {
        qDebug() << "mysql_stmt_init failed";
        return false;
    }

    if (mysql_stmt_prepare(m_stmt, sql, strlen(sql)) != 0) {
        qDebug() << "mysql_stmt_prepare failed:" << mysql_stmt_error(m_stmt);
        CloseStmt();
        return false;
    }

    m_stmtActive = true;
    m_bindParams.clear();
    m_bindResults.clear();
    m_stringBuffer.clear();
    m_paramLengths.clear();
    m_intBuffer.clear();

    return true;
}

bool CMySql::BindParamInt(int paramIndex, int value) {
    if (!m_stmt || !m_stmtActive) return false;

    MYSQL_BIND bind;
    memset(&bind, 0, sizeof(bind));
    bind.buffer_type = MYSQL_TYPE_LONG;
    bind.buffer = &value;
    bind.is_unsigned = 0;
    bind.length = 0;
    bind.is_null = 0;

    m_bindParams.append(bind);
    return true;
}

bool CMySql::BindParamString(int paramIndex, const char* value) {
    if (!m_stmt || !m_stmtActive || !value) return false;

    unsigned long length = strlen(value);
    if (length == 0) return false;

    int bufferPos = m_stringBuffer.size();
    for (unsigned long _i = 0; _i < length; ++_i) m_stringBuffer.append(value[_i]);

    m_paramLengths.append(length);

    MYSQL_BIND bind;
    memset(&bind, 0, sizeof(bind));
    bind.buffer_type = MYSQL_TYPE_STRING;
    bind.buffer = m_stringBuffer.data() + bufferPos;
    bind.buffer_length = length;
    bind.length = &m_paramLengths.last();
    bind.is_null = 0;

    m_bindParams.append(bind);
    return true;
}

bool CMySql::BindParamLongString(int paramIndex, const char* value, unsigned long length) {
    if (!m_stmt || !m_stmtActive) return false;

    int startPos = m_stringBuffer.size();
    for (unsigned long _i = 0; _i < length; ++_i) m_stringBuffer.append(value[_i]);
    m_stringBuffer.append('\0');

    MYSQL_BIND bind;
    memset(&bind, 0, sizeof(bind));
    bind.buffer_type = MYSQL_TYPE_STRING;
    bind.buffer = m_stringBuffer.data() + startPos;
    bind.buffer_length = length;
    bind.is_null = 0;

    m_bindParams.append(bind);
    return true;
}

bool CMySql::ExecuteStmt() {
    if (!m_stmt || !m_stmtActive) return false;

    if (!m_bindParams.isEmpty()) {
        if (mysql_stmt_bind_param(m_stmt, m_bindParams.data()) != 0) {
            qDebug() << "mysql_stmt_bind_param failed:" << mysql_stmt_error(m_stmt);
            return false;
        }
    }

    if (mysql_stmt_execute(m_stmt) != 0) {
        qDebug() << "mysql_stmt_execute failed:" << mysql_stmt_error(m_stmt);
        return false;
    }

    return true;
}

bool CMySql::StoreResult() {
    if (!m_stmt) return false;
    return mysql_stmt_store_result(m_stmt) == 0;
}

bool CMySql::FetchRow() {
    if (!m_stmt) return false;
    return mysql_stmt_fetch(m_stmt) == 0;
}

int CMySql::GetColumnInt(int columnIndex) {
    if (!m_stmt || columnIndex >= m_intBuffer.size()) return 0;
    return m_intBuffer[columnIndex];
}

QByteArray CMySql::GetColumnString(int columnIndex) {
    if (!m_stmt) return "";
    MYSQL_BIND bind;
    memset(&bind, 0, sizeof(bind));

    char buffer[4096];
    unsigned long length;

    bind.buffer_type = MYSQL_TYPE_STRING;
    bind.buffer = buffer;
    bind.buffer_length = sizeof(buffer) - 1;
    bind.length = &length;

    if (mysql_stmt_bind_result(m_stmt, &bind) != 0) {
        return "";
    }

    if (mysql_stmt_fetch_column(m_stmt, &bind, columnIndex, 0) != 0) {
        return "";
    }

    buffer[length] = '\0';
    return QByteArray(buffer);
}

void CMySql::CloseStmt() {
    if (m_stmt) {
        mysql_stmt_close(m_stmt);
        m_stmt = nullptr;
    }
    m_stmtActive = false;
    m_bindParams.clear();
    m_bindResults.clear();
}

bool CMySql::EscapeString(const char* src, char* dest, unsigned long length) {
    if (!m_sock || !src || !dest) return false;
    unsigned long len = strlen(src);
    unsigned long escapedLen = mysql_real_escape_string(m_sock, dest, src, len);
    dest[escapedLen] = '\0';
    return true;
}
