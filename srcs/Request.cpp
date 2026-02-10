#include "../includes/Request.hpp"

Request::Request() : _method(""), _uri(""), _version(""), _body("") {
}

Request::~Request() {
}

bool Request::parse(const std::string& rawRequest) {
    // TODO: Implement parser
    (void)rawRequest;
    return false;
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