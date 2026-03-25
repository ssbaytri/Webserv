#include "../includes/Response.hpp"


Response::Response() : _statusCode(200), _statusMessage("OK"), _body("") {
}

Response::~Response() {
}

void Response::setStatus(int code) {
    _statusCode = code;
    _statusMessage = _getStatusMessage(code);
}

void Response::setHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

void Response::setBody(const std::string& body) {
    _body = body;

    std::stringstream ss;
    ss << body.size();
    setHeader("Content-Length", ss.str());
}

std::string Response::toString() const
{
    std::stringstream ss;

    ss << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";

    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
    {
        ss << it->first << ": " << it->second << "\r\n";
    }

    ss << "\r\n";
    ss << _body;

    return ss.str();
}

std::string Response::_getStatusMessage(int code)
{
    switch(code)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 505: return "HTTP Version Not Supported";
        default: return "Unknown";
    }
}

int Response::getStatusCode() const
{
    return _statusCode;
}

std::string Response::getBody() const
{
    return _body;
}
