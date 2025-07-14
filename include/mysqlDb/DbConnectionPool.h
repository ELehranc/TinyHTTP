#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <thread>
#include "DbConnection.h"
#include <atomic>

class DbConnectionPool
{
public:
    // 单例模式
    static DbConnectionPool &getInstance()
    {
        static DbConnectionPool instance;
        return instance;
    }

    // 初始化连接池
    void init(const std::string &host,
              const std::string &user,
              const std::string &password,
              const std::string &database,
              size_t poolSize = 10);

    // 获取连接
    std::shared_ptr<DbConnection> getConnection();

    // 获取当前连接池中的连接数量
    size_t getConnectionCount() 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return connections_.size();
    }

    // 获取总创建的连接数（包括已释放的）
    int getTotalConnectionCount() const
    {
        return connectionCnt_.load();
    }

    // 打印连接池状态
    void printStatus() 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        LOG_INFO("Connection Pool Status:");
        LOG_INFO("  Current connections: %zu", connections_.size());
        LOG_INFO("  Total created connections: %d", connectionCnt_.load());
    }

private:
    // 构造函数
    DbConnectionPool();
    // 析构函数
    ~DbConnectionPool();

    // 禁止拷贝
    DbConnectionPool(const DbConnectionPool &) = delete;
    DbConnectionPool &operator=(const DbConnectionPool &) = delete;

    std::shared_ptr<DbConnection> createConnection();

    void checkConnections(); // 添加连接检查方法
    // 运行在独立的线程中，专门负责生产新连接
    void produceConnectionTask();

    // 负责扫描检查超时连接
    void scannerConnectionTask();

private:
    std::string host_;
    std::string user_;
    std::string password_;
    std::string database_;
    std::queue<std::shared_ptr<DbConnection>> connections_;
    
    std::mutex mutex_;
    std::condition_variable cv_;
    bool initialized_ = false;
    std::thread checkThread_; // 添加检查线程

    int initSize_;
    int maxSize_;
    int maxIdleTime_;
    int connectionTimeout_;

    std::atomic_int connectionCnt_; // 记录连接创建的总数量
};
