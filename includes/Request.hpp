#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

// Placeholder for now - we'll implement this later
class Request {
private:
    std::string _method;
    std::string _uri;
    std::string _queryString;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::map<std::string, std::string> _cookies;
    std::map<std::string, std::string> _formData;

    std::string host;          // From Host header
    size_t contentLength;      // From Content-Length
    std::string contentType;   // From Content-Type
    std::string connection;    // From Connection
    std::string transferEncoding; // From Transfer-Encoding

    std::string _body;
    size_t _body_start;

    std::string _uploadedFileName;
    std::string _uploadedFileContent;

    bool parseRequestLine(const std::string& rawRequest);
    bool parseHeaders(const std::string& rawRequest);

    bool _parseMultipart();
    std::string _extractBoundary(const std::string& contentType);
    bool _parseChunkedBody(const std::string& rawRequest);
    bool _parseCookies();
    bool _parseUrlEncoded();
    std::string _percentDecode(const std::string& str) const;

public:
    Request();
    ~Request();
    
    // Parser (to be implemented)
    bool parse(const std::string& rawRequest);
    
    // Getters
    std::string getMethod() const;
    std::string getUri() const;
    std::string getVersion() const;
    size_t getContentLength() const;
    std::string getContentType() const;
    std::string getBody() const;

    bool isMultipartUpload() const;
    std::string getUploadedFileName() const;
    std::string getUploadedFileContent() const;
    std::string getTransferEncoding() const;
    std::string getQueryString() const;
    std::string getConnection() const;

    std::string getCookie(const std::string& name) const;
    std::map<std::string, std::string> getCookies() const;

    std::string getFormField(const std::string& name) const;
    bool isUrlEncodedForm() const;
};

#endif