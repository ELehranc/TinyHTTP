#include "Session.h"
#include "SessionManager.h"

Session::Session(const std::string &sessionId,
                 SessionManager *sessionManager, int maxAge) : sessionId_(sessionId), maxAge_(maxAge), sessionManager_(sessionManager)
{
    refresh();
}

bool Session::isExpired() const
{
    return std::chrono::system_clock::now() > expiryTime_;
}

void Session::refresh()
{
    expiryTime_ = std::chrono::system_clock::now() + std::chrono::seconds(maxAge_);
}

void Session::setValue(const std::string &key, const std::string &value)
{

    data_[key] = value;

    if (sessionManager_)
    {
        sessionManager_->updateSession(shared_from_this());
    }
}

std::string Session::getValue(const std::string &key) const
{
    auto it = data_.find(key);
    return it != data_.end() ? it->second : std::string();
}

void Session::remove(const std::string &key)
{
    data_.erase(key);
}

void Session::clear()
{
    data_.clear();
}
