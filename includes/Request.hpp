#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

// Placeholder for now - we'll implement this later
class Request {
private:
    std::string _method;
    std::string _uri;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;

public:
    Request();
    ~Request();
    
    // Parser (to be implemented)
    bool parse(const std::string& rawRequest);
    
    // Getters
    std::string getMethod() const;
    std::string getUri() const;
    std::string getVersion() const;
};

#endif