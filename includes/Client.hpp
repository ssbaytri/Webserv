#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>
#include <cstdlib>

enum ClientState {
    READING_REQUEST,
    SENDING_RESPONSE,
    DONE
};

class Client {
	private:
		int             _fd;
		std::string     _requestBuffer;
		std::string     _responseBuffer;
		ClientState     _state;
		time_t          _lastActivity;
		size_t          _bytesSent;

	public:
		// Constructor & Destructor
		Client(int fd);
		~Client();

		// Request handling
		void            appendToRequest(const std::string& data);
		bool            isRequestComplete() const;
		std::string     getRequest() const;
		void            clearRequest();

		// Response handling
		void            setResponse(const std::string& response);
		std::string     getResponseChunk(size_t maxSize);
		bool            hasMoreToSend() const;

		// State management
		ClientState     getState() const;
		void            setState(ClientState state);

		// Activity tracking
		void            updateActivity();
		time_t          getLastActivity() const;
		bool            isTimedOut(time_t timeout) const;

		// Getters
		int             getFd() const;
};

#endif