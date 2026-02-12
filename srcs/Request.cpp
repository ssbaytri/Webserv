#include "../includes/Request.hpp"
#include "../includes/utils.hpp"
#include <iostream>
#include <cstdlib>

Request::Request() 
    : _method(""), 
    _uri(""), 
    _version(""), 
    _body(""),
    host(""),
    contentLength(0),
    contentType(""),
    connection(""),
    _body_start(0)
{
}

Request::~Request() {
}

bool Request::parseRequestLine(const std::string& rawRequest)
{
    size_t lineEnd = rawRequest.find("\r\n");
    if (lineEnd == std::string::npos) {
        logError("No \\r\\n found in request");
        return false;
    }
    
    std::string requestLine = rawRequest.substr(0, lineEnd);
    std::cout << "Request Line: [" << requestLine << "]" << std::endl;
    
    size_t firstSpace = requestLine.find(' ');
    if (firstSpace == std::string::npos) {
        logError("No space after method");
        return false;
    }
    
    _method = requestLine.substr(0, firstSpace);
    std::cout << "Method: [" << _method << "]" << std::endl;
    
    size_t secondSpace = requestLine.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos) {
        logError("No space after URI");
        return false;
    }
    
    _uri = requestLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    std::cout << "URI: [" << _uri << "]" << std::endl;
    
    _version = requestLine.substr(secondSpace + 1);
    std::cout << "Version: [" << _version << "]" << std::endl;
    
    if (_method.empty() || _uri.empty() || _version.empty()) {
        logError("Empty field in request line");
        return false;
    }
    
    if (_version != "HTTP/1.1" && _version != "HTTP/1.0") {
        logError("Unsupported HTTP version: " + std::string(_version));
        return false;
    }
    
    return true;
}

bool Request::parseHeaders(const std::string& rawRequest)
{
    size_t pos = rawRequest.find("\r\n");
    if (pos == std::string::npos)
    {
        logError("No request line found");
        return false;
    }

    pos += 2;
    while (pos < rawRequest.length())
    {
        size_t line_end = rawRequest.find("\r\n", pos);
        if (line_end == std::string::npos)
        {
            logError("Malformed headers no (\\r\\n)");
            return false;
        }

        if (line_end == pos)
        {
            _body_start = pos + 2;
            std::cout << "End of headers found at position " << _body_start << std::endl;
            return true;
        }

        std::string line = rawRequest.substr(pos, line_end - pos);
        std::cout << "Parsing header line: [" << line << "]" << std::endl;

        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos)
        {
            logError("Invalid header (No colon)");
            return false;
        }

        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);

        key = trim(key);
        value = trim(value);

        key = toLowerCase(key);

        _headers[key] = value;

        pos = line_end + 2;
    }
    logError("Error: No empty line found (\\r\\n\\r\\n)");
    return false;
}

bool Request::parse(const std::string& rawRequest)
{
    if (!parseRequestLine(rawRequest) || !parseHeaders(rawRequest))
    {
        logError("Invalid HTTP request");
        return false;
    }

    host = _headers["host"];
    contentLength = atoi(_headers["content-length"].c_str());
    contentType = _headers["content-type"];

    std::cout << "Host: " << host << std::endl;
    std::cout << "Content-Length: " << contentLength << std::endl;
    std::cout << "Content-type: " << contentType << std::endl;

    return true;
}

std::string Request::getMethod() const {
    return _method;
}

std::string Request::getUri() const {
    return _uri;
}

std::string Request::getVersion() const {
    return _version;
}