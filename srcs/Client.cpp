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
    // First, check if we have the headers (ends with \r\n\r\n)
    size_t headerEnd = _requestBuffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return false; // Headers not complete yet
    }
    
    // Headers are complete, now check if we need a body
    // Extract Content-Length from headers (simple parsing)
    size_t contentLengthPos = _requestBuffer.find("Content-Length:");
    if (contentLengthPos == std::string::npos) {
        contentLengthPos = _requestBuffer.find("content-length:");
    }
    
    if (contentLengthPos != std::string::npos && contentLengthPos < headerEnd) {
        // Find the value
        size_t valueStart = _requestBuffer.find(":", contentLengthPos) + 1;
        size_t valueEnd = _requestBuffer.find("\r\n", valueStart);
        
        if (valueStart != std::string::npos && valueEnd != std::string::npos) {
            std::string lengthStr = _requestBuffer.substr(valueStart, valueEnd - valueStart);
            
            // Trim whitespace
            size_t first = lengthStr.find_first_not_of(" \t");
            size_t last = lengthStr.find_last_not_of(" \t");
            if (first != std::string::npos && last != std::string::npos) {
                lengthStr = lengthStr.substr(first, last - first + 1);
            }
            
            int contentLength = atoi(lengthStr.c_str());
            
            if (contentLength > 0) {
                // Body starts after \r\n\r\n
                size_t bodyStart = headerEnd + 4;
                size_t bodyReceived = _requestBuffer.length() - bodyStart;
                
                // Check if we've received the entire body
                return bodyReceived >= (size_t)contentLength;
            }
        }
    }
    
    // No Content-Length or Content-Length: 0, headers are enough
    return true;
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