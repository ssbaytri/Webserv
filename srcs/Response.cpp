#include "../includes/Response.hpp"

Response::Response() : _statusCode(200), _statusMessage("OK"), _body("") {
}

Response::~Response() {
}

void Response::setStatus(int code) {
    _statusCode = code;
    // TODO: Set proper status message based on code
}

void Response::setHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

void Response::setBody(const std::string& body) {
    _body = body;
}

std::string Response::toString() const {
    // TODO: Build proper HTTP response
    return "";
}