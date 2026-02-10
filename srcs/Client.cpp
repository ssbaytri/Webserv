#include "../includes/Client.hpp"

// Constructor
Client::Client(int fd) 
    : _fd(fd),
      _requestBuffer(""),
      _responseBuffer(""),
      _state(READING_REQUEST),
      _lastActivity(std::time(NULL)),
      _bytesSent(0) {
}

// Destructor
Client::~Client() {
    // fd will be closed by Server, not here
}

// Request handling
void Client::appendToRequest(const std::string& data) {
    _requestBuffer += data;
    updateActivity();
}

bool Client::isRequestComplete() const {
    // Simple check: HTTP request ends with \r\n\r\n (empty line)
    // For now, we'll just check for this pattern
    // Later, we'll need to handle Content-Length for POST requests
    return _requestBuffer.find("\r\n\r\n") != std::string::npos;
}

std::string Client::getRequest() const {
    return _requestBuffer;
}

void Client::clearRequest() {
    _requestBuffer.clear();
}

// Response handling
void Client::setResponse(const std::string& response) {
    _responseBuffer = response;
    _bytesSent = 0;
    _state = SENDING_RESPONSE;
}

std::string Client::getResponseChunk(size_t maxSize) {
    if (_bytesSent >= _responseBuffer.size()) {
        return "";
    }
    
    size_t remaining = _responseBuffer.size() - _bytesSent;
    size_t chunkSize = (remaining < maxSize) ? remaining : maxSize;
    
    std::string chunk = _responseBuffer.substr(_bytesSent, chunkSize);
    _bytesSent += chunkSize;
    
    return chunk;
}

bool Client::hasMoreToSend() const {
    return _bytesSent < _responseBuffer.size();
}

// State management
ClientState Client::getState() const {
    return _state;
}

void Client::setState(ClientState state) {
    _state = state;
}

// Activity tracking
void Client::updateActivity() {
    _lastActivity = std::time(NULL);
}

time_t Client::getLastActivity() const {
    return _lastActivity;
}

bool Client::isTimedOut(time_t timeout) const {
    return (std::time(NULL) - _lastActivity) > timeout;
}

// Getters
int Client::getFd() const {
    return _fd;
}