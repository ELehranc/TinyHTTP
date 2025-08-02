#pragma once
#include <memory>
#include <string>
#include <mutex>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <mysql_driver.h>
#include <mysql/mysql.h>
#include <netsocket/base/Logger.h>
#include <netsocket/base/Timestamp.h>
#include "DbException.h"

class DbConnection
{
public:
    DbConnection(const std::string &host,
                 const std::string &user,
                 const std::string &password,
                 const std::string &database);
    ~DbConnection();

    // 禁止拷贝
    DbConnection(const DbConnection &) = delete;
    DbConnection &operator=(const DbConnection &) = delete;

    bool isValid();
    void reconnect();
    void cleanup();

    template <typename... Args>
    sql::ResultSet *executeQuery(const std::string &sql, Args &&...args)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            // 直接创建新的预处理语句，不使用缓存
            std::unique_ptr<sql::PreparedStatement> stmt(conn_->prepareStatement(sql));
            bindParams(stmt.get(), 1, std::forward<Args>(args)...);
            return stmt->executeQuery();
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR("Query failed: %s, SQL: %s", e.what(), sql);
            throw DbException(e.what());
        }
    }

    bool execute(const std::string &sql)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            // 直接创建新的预处理语句，不使用缓存
            std::unique_ptr<sql::PreparedStatement> stmt(conn_->prepareStatement(sql));
            return stmt->execute();
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR("Excute failed: %s, SQL: %s", e.what(), sql);
            throw DbException(e.what());
        }
    }

    template <typename... Args>
    int executeUpdate(const std::string &sql, Args &&...args)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            // 直接创建新的预处理语句，不使用缓存
            std::unique_ptr<sql::PreparedStatement> stmt(
                conn_->prepareStatement(sql));
            bindParams(stmt.get(), 1, std::forward<Args>(args)...);
            return stmt->executeUpdate();
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR("Update failed: %s, SQL: %s", e.what(), sql);
            throw DbException(e.what());
        }
    }

    bool ping(); // 添加检测连接是否有效的方法

    // 刷新连接的起始事件
    void reflashAliveTime() { alivetime_ = Timestamp::now(); }
    int64_t getAliveTime() { return Timestamp::now() - alivetime_; }

private:
    // 辅助函数：递归终止条件
    void bindParams(sql::PreparedStatement *, int) {}

    // 辅助函数：绑定参数
    template <typename T, typename... Args>
    void bindParams(sql::PreparedStatement *stmt, int index,
                    T &&value, Args &&...args)
    {
        stmt->setString(index, std::to_string(std::forward<T>(value)));
        bindParams(stmt, index + 1, std::forward<Args>(args)...);
    }

    // 特化 string 类型的参数绑定
    template <typename... Args>
    void bindParams(sql::PreparedStatement *stmt, int index,
                    const std::string &value, Args &&...args)
    {
        stmt->setString(index, value);
        bindParams(stmt, index + 1, std::forward<Args>(args)...);
    }



private:
    std::shared_ptr<sql::Connection> conn_;
    std::string host_;
    std::string user_;
    std::string password_;
    std::string database_;
    std::mutex mutex_;

    Timestamp alivetime_; // 记录空间状态的存活事件
};
