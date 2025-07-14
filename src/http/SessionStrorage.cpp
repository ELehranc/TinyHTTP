#include "SessionStorage.h"
#include "iostream"

void MemorySessionStorage::save(std::shared_ptr<Session> session)
{

    sessions_[session->getId()] = session;
}

std::shared_ptr<Session> MemorySessionStorage::load(const std::string &sessionId)
{

    auto it = sessions_.find(sessionId);
    if (it != sessions_.end())
    {

        if (!it->second->isExpired())
        {
            return it->second;
        }
        else
        {
            sessions_.erase(it);
        }
    }
    return nullptr;
}

void MemorySessionStorage::remove(const std::string &sessionId)
{
    sessions_.erase(sessionId);
}
