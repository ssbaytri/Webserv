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

bool Client::isRequestComplete() const
{
    // 1. Check if headers are complete
    size_t headersEnd = _requestBuffer.find("\r\n\r\n");
    if (headersEnd == std::string::npos) {
        return false;  // Headers not done yet
    }
    
    // 2. Check if there's a Content-Length header
    size_t clPos = _requestBuffer.find("Content-Length:");
    if (clPos == std::string::npos || clPos > headersEnd) {
        // No Content-Length, request is complete after headers
        return true;
    }
    
    // 3. Extract Content-Length value
    size_t lineEnd = _requestBuffer.find("\r\n", clPos);
    std::string clLine = _requestBuffer.substr(clPos, lineEnd - clPos);
    size_t colonPos = clLine.find(':');
    std::string lengthStr = clLine.substr(colonPos + 1);
    
    // Trim whitespace
    size_t start = lengthStr.find_first_not_of(" \t");
    size_t end = lengthStr.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return true;
    lengthStr = lengthStr.substr(start, end - start + 1);
    
    size_t contentLength = std::atoi(lengthStr.c_str());
    
    // 4. Check if we have the full body
    size_t bodyStart = headersEnd + 4;
    size_t receivedBody = _requestBuffer.length() - bodyStart;
    
    return receivedBody >= contentLength;
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