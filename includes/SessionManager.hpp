#pragma once

#include <string>
#include <map>
#include <ctime>

struct SessionData
{
	std::string username;
	time_t		createdAt;
	time_t		lastAccessed;
};

class SessionManager
{
	private:
		std::map<std::string, SessionData> _sessions;
		std::string _generateId() const;

	public:
		SessionManager();
		~SessionManager();

		std::string     createSession(const std::string& username);
		SessionData*    getSession(const std::string& sessionId);
		void            destroySession(const std::string& sessionId);
		void            cleanExpiredSessions(time_t maxAge);
};
