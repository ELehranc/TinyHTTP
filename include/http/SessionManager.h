#pragma once

#include "SessionStorage.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <memory>
#include <random>

class SessionManager
{

public:
    explicit SessionManager(std::unique_ptr<SessionStorage> storage);

    std::shared_ptr<Session> getSession(const HttpRequest &req, HttpResponse *resp);

    void destroySession(const std::string &sessionId);

    void cleanExpiredSessions();

    void updateSession(std::shared_ptr<Session> session) { storage_->save(session); }

private:
    std::string generateSessionId();
    std::string generateSessionId2();
    std::string getSessionIdFromCookie(const HttpRequest &req);
    void setSessionCookie(const std::string &sessionId, HttpResponse *resp);

private:
    std::unique_ptr<SessionStorage> storage_;
    std::mt19937 rng_;
};
