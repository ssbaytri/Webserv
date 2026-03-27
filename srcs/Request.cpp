#include "../includes/Request.hpp"
#include "../includes/utils.hpp"
#include <iostream>
#include <cstdlib>

Request::Request() 
    : _method(""), 
    _uri(""), 
    _version(""), 
    host(""),
    contentLength(0),
    contentType(""),
    connection(""),
    transferEncoding(""),
    _body(""),
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

    size_t queryPos = _uri.find("?");
    if (queryPos != std::string::npos)
    {
        _queryString = _uri.substr(queryPos + 1);
        _uri = _uri.substr(0, queryPos);
    }
    else
        _queryString = "";

    std::cout << "URI: [" << _uri << "]" << std::endl;
    std::cout << "QuertString: [" << _queryString << "]" << std::endl;
    
    _version = requestLine.substr(secondSpace + 1);
    std::cout << "Version: [" << _version << "]" << std::endl;
    
    if (_method.empty() || _uri.empty() || _version.empty()) {
        logError("Empty field in request line");
        return false;
    }
    
    if (_version != "HTTP/1.1") 
    {
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
    transferEncoding = _headers["transfer-encoding"];
    connection = _headers["connection"];

    // Handle chunked transfer encoding
    if (transferEncoding == "chunked")
    {
        if (!_parseChunkedBody(rawRequest))
        {
            logError("Failed to parse chunked body");
            return false;
        }
    }
    else if (contentLength > 0 && _body_start != std::string::npos)
    {
        size_t availableBody = rawRequest.length() - _body_start;
        if (availableBody < contentLength) {
            logError("Incomplete body! Expected: " + intToString(contentLength) + 
                    ", Got: " + intToString(availableBody));
            return false;
        }
        _body = rawRequest.substr(_body_start, contentLength);
    }

    std::cout << "Host: " << host << std::endl;
    std::cout << "Content-Length: " << contentLength << std::endl;
    std::cout << "Content-type: " << contentType << std::endl;

    if (isMultipartUpload()) {
        if (!_parseMultipart()) {
            logError("Failed to parse multipart upload");
            return false;
        }
    }

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

size_t Request::getContentLength() const
{
    return contentLength;
}

std::string Request::getContentType() const
{
    return contentType;
}

std::string Request::getBody() const
{
    return _body;
}

std::string Request::_extractBoundary(const std::string& contentType)
{
    // contentType looks like: "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW"
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        return "";
    }
    return contentType.substr(boundaryPos + 9);  // 9 = strlen("boundary=")
}

bool Request::_parseMultipart()
{
    std::string boundary = _extractBoundary(contentType);
    if (boundary.empty()) {
        logError("No boundary found in Content-Type");
        return false;
    }

    // The boundary in the body already has -- prefix
    std::string boundaryDelimiter = "--" + boundary;
    size_t pos = _body.find(boundaryDelimiter);
    
    if (pos == std::string::npos) {
        logError("Boundary not found in multipart body");
        logError("Looking for: [" + boundaryDelimiter + "]");
        return false;
    }

    // Move past first boundary and \r\n
    pos += boundaryDelimiter.length();
    if (pos + 1 < _body.length() && _body[pos] == '\r' && _body[pos + 1] == '\n') {
        pos += 2;
    }

    // Parse part headers
    size_t headerEnd = _body.find("\r\n\r\n", pos);
    if (headerEnd == std::string::npos) {
        logError("Part headers not found");
        return false;
    }

    std::string partHeaders = _body.substr(pos, headerEnd - pos);
    std::cout << "Part headers:\n" << partHeaders << std::endl;

    // Extract filename
    size_t filenamePos = partHeaders.find("filename=\"");
    if (filenamePos != std::string::npos) {
        filenamePos += 10;
        size_t filenameEnd = partHeaders.find("\"", filenamePos);
        if (filenameEnd != std::string::npos) {
            _uploadedFileName = partHeaders.substr(filenamePos, filenameEnd - filenamePos);
            std::cout << "Extracted filename: " << _uploadedFileName << std::endl;
        }
    }

    // File content starts after \r\n\r\n
    size_t contentStart = headerEnd + 4;

    // Find the closing boundary (with \r\n prefix)
    std::string closingBoundary = "\r\n" + boundaryDelimiter;
    size_t contentEnd = _body.find(closingBoundary, contentStart);
    
    if (contentEnd == std::string::npos) {
        // Try without \r\n prefix (edge case)
        contentEnd = _body.find(boundaryDelimiter, contentStart);
        if (contentEnd == std::string::npos) {
            logError("End boundary not found");
            logError("Body length: " + intToString(_body.length()));
            logError("Content start: " + intToString(contentStart));
            return false;
        }
        _uploadedFileContent = _body.substr(contentStart, contentEnd - contentStart);
    } else {
        _uploadedFileContent = _body.substr(contentStart, contentEnd - contentStart);
    }

    std::cout << "File size: " << _uploadedFileContent.size() << " bytes" << std::endl;

    if (_uploadedFileName.empty() || _uploadedFileContent.empty()) {
        logError("Failed to extract filename or content");
        return false;
    }

    return true;
}

bool Request::isMultipartUpload() const
{
    return contentType.find("multipart/form-data") != std::string::npos;
}

std::string Request::getUploadedFileName() const
{
    return _uploadedFileName;
}

std::string Request::getUploadedFileContent() const
{
    return _uploadedFileContent;
}

std::string Request::getTransferEncoding() const
{
    return transferEncoding;
}

std::string Request::getQueryString() const
{
    return _queryString;
}

std::string Request::getConnection() const
{
    return connection;
}

bool Request::_parseChunkedBody(const std::string& rawRequest)
{
    if (_body_start == std::string::npos)
    {
        logError("Body start position not found");
        return false;
    }

    std::cout << "Parsing chunked body..." << std::endl;
    
    size_t pos = _body_start;
    std::string decodedBody;
    
    while (pos < rawRequest.length())
    {
        // Find the end of the chunk size line
        size_t chunkSizeEnd = rawRequest.find("\r\n", pos);
        if (chunkSizeEnd == std::string::npos)
        {
            logError("Malformed chunk: no CRLF after chunk size");
            return false;
        }
        
        // Extract chunk size (in hexadecimal)
        std::string chunkSizeStr = rawRequest.substr(pos, chunkSizeEnd - pos);
        
        // Handle chunk extensions (e.g., "1a;name=value") - ignore everything after ';'
        size_t semicolonPos = chunkSizeStr.find(';');
        if (semicolonPos != std::string::npos)
        {
            chunkSizeStr = chunkSizeStr.substr(0, semicolonPos);
        }
        
        // Trim whitespace
        chunkSizeStr = trim(chunkSizeStr);
        
        std::cout << "Chunk size (hex): [" << chunkSizeStr << "]" << std::endl;
        
        // Convert hex string to integer
        size_t chunkSize = 0;
        for (size_t i = 0; i < chunkSizeStr.length(); i++)
        {
            char c = chunkSizeStr[i];
            chunkSize *= 16;
            if (c >= '0' && c <= '9')
                chunkSize += c - '0';
            else if (c >= 'a' && c <= 'f')
                chunkSize += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                chunkSize += c - 'A' + 10;
            else
            {
                logError("Invalid hex character in chunk size: " + std::string(1, c));
                return false;
            }
        }
        
        std::cout << "Chunk size (decimal): " << chunkSize << std::endl;
        
        // Check if this is the last chunk (size 0)
        if (chunkSize == 0)
        {
            std::cout << "Last chunk received. Total body size: " << decodedBody.length() << std::endl;
            _body = decodedBody;
            contentLength = decodedBody.length();
            return true;
        }
        
        // Move past the chunk size line (past \r\n)
        pos = chunkSizeEnd + 2;
        
        // Check if we have enough data for the chunk
        if (pos + chunkSize > rawRequest.length())
        {
            logError("Incomplete chunk data");
            return false;
        }
        
        // Extract chunk data
        std::string chunkData = rawRequest.substr(pos, chunkSize);
        decodedBody += chunkData;
        
        // Move past the chunk data
        pos += chunkSize;
        
        // Each chunk should end with \r\n
        if (pos + 2 > rawRequest.length() || 
            rawRequest[pos] != '\r' || rawRequest[pos + 1] != '\n')
        {
            logError("Chunk data not followed by CRLF");
            return false;
        }
        
        // Move past the trailing \r\n
        pos += 2;
    }
    
    logError("Chunked body ended without terminating chunk (0-sized chunk)");
    return false;
}
