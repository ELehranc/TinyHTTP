// MysqlUtil.h
#pragma once

#include <mysqlconn/DbConnectionPool.h>
#include <string>
#include <memory>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <stdexcept>

class MysqlUtil
{
public:
    static void init(const std::string &host,const std::string &user,const std::string &password,const std::string &database,size_t poolsize=10)
    {
        DbConnectionPool::getInstance().init(host,user,password,database,poolsize);
    }

    template <typename... Args>
    sql::ResultSet *executeQuery(const std::string &sql, Args &&...args)
    {
        auto conn = DbConnectionPool::getInstance().getConnection();
        return conn->executeQuery(sql, std::forward<Args>(args)...);
    }

    
    bool execute(const std::string &sql)
    {
        auto conn = DbConnectionPool::getInstance().getConnection();
        return conn->execute(sql);
    }

    template <typename... Args>
    int executeUpdate(const std::string &sql, Args &&...args)
    {
        auto conn = DbConnectionPool::getInstance().getConnection();
        return conn->executeUpdate(sql, std::forward<Args>(args)...);
    }
};
