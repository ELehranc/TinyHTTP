#include "DbConnectionPool.h"
#include "DbException.h"
#include <mymuduo/Logger.h>

void DbConnectionPool::init(const std::string &host,
                            const std::string &user,
                            const std::string &password,
                            const std::string &database,
                            size_t poolSize)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_)
    {
        LOG_INFO("连接池已初始化，跳过");
        return;
    }

    host_ = host;
    user_ = user;
    password_ = password;
    database_ = database;

    initSize_ = 10;
    maxSize_ = 1024;
    maxIdleTime_ = 60;
    connectionTimeout_ = 100;

    // 记录创建连接前的状态
    //  LOG_INFO("开始创建 %zu 个初始连接", poolSize);

    for (size_t i = 0; i < poolSize; ++i)
    {
        auto conn = createConnection();
        if (conn)
        {
            connections_.push(conn);
            //  LOG_INFO("创建连接 %zu 成功", i + 1);
        }
        else
        {
            //  LOG_ERROR("创建连接 %zu 失败", i + 1);
            // 若创建失败，是否继续？建议抛出异常终止初始化
            throw DbException("初始化连接池时创建连接失败");
        }
    }

    initialized_ = true;
    //  LOG_INFO("连接池初始化完成，当前连接数：%zu", connections_.size());
}

DbConnectionPool::DbConnectionPool()
{
    checkThread_ = std::thread(&DbConnectionPool::checkConnections, this);
    checkThread_.detach();

    // 启动一个新的线程，作为连接的生产者
    std::thread producer(std::bind(&DbConnectionPool::produceConnectionTask, this)); // 注意这里的处理，我们通过绑定成员函数，可以避免将produce声明成成员函数，又不妨碍线程调用类的成员方法
    std::thread scanner(std::bind(&DbConnectionPool::scannerConnectionTask, this));

    producer.detach();
    scanner.detach();
}

DbConnectionPool::~DbConnectionPool()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        initialized_ = false;
        cv_.notify_all();
    }
    if (checkThread_.joinable())
    {
        checkThread_.join(); // 等待检查线程完成当前任务后退出
    }
    while (!connections_.empty())
    {
        connections_.pop();
    }
    LOG_INFO("Database connection pool destroyed");
}

// 修改获取连接的函数
std::shared_ptr<DbConnection> DbConnectionPool::getConnection()
{
    std::shared_ptr<DbConnection> conn;
    {
        std::unique_lock<std::mutex> lock(mutex_);

        while (connections_.empty())
        {
            if (!initialized_)
            {
                throw DbException("Connection pool not initialized");
            }
            // LOG_INFO("Waiting for available connection...");
            cv_.wait(lock);
        }

        conn = connections_.front();
        connections_.pop();
    } // 释放锁

    try
    {
        // 在锁外检查连接
        if (!conn->ping())
        {
            LOG_ERROR("Connection lost, attempting to reconnect...");
            conn->reconnect();
        }

        return std::shared_ptr<DbConnection>(conn.get(),
                                             [this, conn](DbConnection *)
                                             {
                                                 std::lock_guard<std::mutex> lock(mutex_);
                                                 connections_.push(conn);
                                                 cv_.notify_one();
                                             });
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Failed to get connection: %s", e.what());
        {
            std::lock_guard<std::mutex> lock(mutex_);
            connections_.push(conn);
            cv_.notify_one();
        }
        throw;
    }
}

std::shared_ptr<DbConnection> DbConnectionPool::createConnection()
{
    try
    {
        auto conn = std::make_shared<DbConnection>(host_, user_, password_, database_);
        //   LOG_INFO("成功创建数据库连接");
        return conn;
    }
    catch (const sql::SQLException &e)
    {
        LOG_ERROR("创建连接失败(SQL异常):%s", e.what());
        return nullptr;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("创建连接失败（未知异常）：%s", e.what());
        return nullptr;
    }
}

void DbConnectionPool::scannerConnectionTask()
{
    for (;;)
    {

        std::this_thread::sleep_for(std::chrono::seconds(maxIdleTime_));

        std::unique_lock<std::mutex> lock(mutex_);
        while (connectionCnt_ > initSize_)
        {

            auto p = connections_.front();
            if (p->getAliveTime() >= (maxIdleTime_ * 1000))
            {
                LOG_ERROR("删除一个空闲连接噜");
                p.reset(); // 释放连接
                connections_.pop();
            }
            else
            {
                break;
            }
        }
    }
}

void DbConnectionPool::produceConnectionTask()
{

    for (;;)
    {

        std::unique_lock<std::mutex> lock(mutex_);
        while (!connections_.empty())
        {
            cv_.wait(lock); // 还有连接，就一直在这里等待
        }

        // 连接数未达到上限，继续创建连接
        if (connectionCnt_ < maxSize_)
        {

            auto p = createConnection();
            if (p)
            {

                connections_.push(p);
                p->reflashAliveTime();
                connectionCnt_++;
            }
        }
        // 通知消费者线程，可以获取连接了
        cv_.notify_all();
    }
}

// 修改检查连接的函数
void DbConnectionPool::checkConnections()
{
    while (initialized_)
    {
        try
        {
            std::vector<std::shared_ptr<DbConnection>> connsToCheck;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (connections_.empty())
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }

                auto temp = connections_;
                while (!temp.empty())
                {
                    connsToCheck.push_back(temp.front());
                    temp.pop();
                }
            }

            // 在锁外检查连接
            for (auto &conn : connsToCheck)
            {
                if (!conn->ping())
                {
                    try
                    {
                        conn->reconnect();
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR("Failed to reconnect: %s", e.what());
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Error in check thread: %s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}
