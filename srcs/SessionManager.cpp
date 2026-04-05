#include "../includes/SessionManager.hpp"
#include <cstdlib>
#include <sstream>

SessionManager::SessionManager() {}
SessionManager::~SessionManager() {}

std::string SessionManager::_generateId() const
{
    // combine timestamp + random number to make a unique ID
    std::stringstream ss;
    ss << std::hex << std::time(NULL) << std::rand() << std::rand();
    return ss.str();
}

std::string SessionManager::createSession(const std::string& username)
{
    std::string sessionId = _generateId();

    SessionData data;
    data.username     = username;
    data.createdAt    = std::time(NULL);
    data.lastAccessed = std::time(NULL);

    _sessions[sessionId] = data;

    return sessionId;
}

SessionData* SessionManager::getSession(const std::string& sessionId)
{
    std::map<std::string, SessionData>::iterator it = _sessions.find(sessionId);
    if (it == _sessions.end())
        return NULL;

    // update last accessed time
    it->second.lastAccessed = std::time(NULL);

    return &it->second;
}

void SessionManager::destroySession(const std::string& sessionId)
{
    _sessions.erase(sessionId);
}

void SessionManager::cleanExpiredSessions(time_t maxAge)
{
    time_t now = std::time(NULL);

    std::map<std::string, SessionData>::iterator it = _sessions.begin();
    while (it != _sessions.end())
    {
        if (now - it->second.lastAccessed > maxAge)
        {
            std::map<std::string, SessionData>::iterator toErase = it;
            ++it;
            _sessions.erase(toErase);
        }
        else
            ++it;
    }
}
