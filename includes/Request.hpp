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

    std::string host;          // From Host header
    size_t contentLength;      // From Content-Length
    std::string contentType;   // From Content-Type
    std::string connection;    // From Connection

    std::string _body;
    size_t _body_start;

    bool parseRequestLine(const std::string& rawRequest);
    bool parseHeeaders(const std::string& rawRequest);

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