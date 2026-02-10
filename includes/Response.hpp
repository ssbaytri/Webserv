#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

// Placeholder for now - we'll implement this later
class Response {
private:
    int _statusCode;
    std::string _statusMessage;
    std::map<std::string, std::string> _headers;
    std::string _body;

public:
    Response();
    ~Response();
    
    // Builder methods (to be implemented)
    void setStatus(int code);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    
    // Generate HTTP response string
    std::string toString() const;
};

#endif