#include "../includes/Server.hpp"
#include "../includes/utils.hpp"
#include "../includes/Request.hpp"
#include "../includes/Response.hpp"
#include <arpa/inet.h>
#include <stdexcept>
#include <fstream>

// Constructor
Server::Server(const ServerConfig& config) : _config(config), _serverSocket(-1) {
    _setupSocket();
}

// Destructor
Server::~Server() {
    // Clean up all clients
    for (std::map<int, Client*>::iterator it = _clients.begin();
         it != _clients.end(); ++it) {
        delete it->second;
        close(it->first);
    }
    _clients.clear();
    
    // Close server socket
    if (_serverSocket != -1) {
        close(_serverSocket);
    }
    
    logMessage("Server shut down");
}

// Setup socket
void Server::_setupSocket() {
    struct sockaddr_in serverAddr;
    
    // 1. Create socket
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // 2. Set socket options (SO_REUSEADDR to avoid "Address already in use")
    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(_serverSocket);
        throw std::runtime_error("Failed to set socket options");
    }
    
    // 3. Set non-blocking mode
    _setNonBlocking(_serverSocket);
    
    // 4. Setup address structure
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    serverAddr.sin_port = htons(_config.port);
    
    // 5. Bind socket to address
    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(_serverSocket);
        throw std::runtime_error("Failed to bind socket to port " + intToString(_config.port));
    }
    
    // 6. Start listening
    if (listen(_serverSocket, 128) < 0) {  // 128 is backlog size
        close(_serverSocket);
        throw std::runtime_error("Failed to listen on socket");
    }
    
    // 7. Add server socket to poll
    struct pollfd serverPollFd;
    serverPollFd.fd = _serverSocket;
    serverPollFd.events = POLLIN;  // Monitor for incoming connections
    serverPollFd.revents = 0;
    _pollFds.push_back(serverPollFd);
    
    logMessage("Server listening on port " + intToString(_config.port));
}

// Set socket to non-blocking mode
void Server::_setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        throw std::runtime_error("Failed to get socket flags");
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }
}

// Accept new connection
void Server::_acceptNewConnection() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSocket = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
    
    if (clientSocket < 0)
        return;
    
    // Set client socket to non-blocking
    _setNonBlocking(clientSocket);
    
    // Get client IP for logging
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    logMessage("New connection from " + std::string(clientIP) + 
               " on socket " + intToString(clientSocket));
    
    // Create Client object
    Client* newClient = new Client(clientSocket);
    _clients[clientSocket] = newClient;
    
    // Add to poll
    struct pollfd clientPollFd;
    clientPollFd.fd = clientSocket;
    clientPollFd.events = POLLIN;  // Start by reading the request
    clientPollFd.revents = 0;
    _pollFds.push_back(clientPollFd);
}

void Server::_handleGET(const Request& request, Response& response) {
    std::string uri = request.getUri();
    
    std::string root = _config.root;
    const LocationConfig* location = _findLocation(uri);

    if (location && !location->root.empty())
        root = location->root;

    std::string filepath = root + uri;

    if (uri == "/" || uri[uri.length() - 1] == '/')
    {
        std::vector<std::string> indexFiles;

        if (location && !location->index.empty())
            indexFiles = location->index;
        else if (!_config.index.empty())
            indexFiles = _config.index;
        else
            indexFiles.push_back("index.html");

        bool indexFound = false;
        for (size_t i = 0; i < indexFiles.size(); i++)
        {
            std::string indexPath = filepath + indexFiles[i];
            if (fileExists(indexPath))
            {
                filepath = indexPath;
                indexFound = true;
                break;
            }
        }

        if (!indexFound)
        {
            if (location && location->autoindex)
            {
                // TODO: Directory listing;
                response.setStatus(200);
                response.setBody("<h1>Directory listing</h1>");
                return ;
            }
            else
            {
                _setErrorResponse(response, 403);
                return ;
            }
        }
    }
    
    logMessage("Serving file: " + filepath);
    
    // Check if file exists
    if (fileExists(filepath)) {
        std::string content = readFile(filepath);
        
        if (!content.empty()) {
            response.setStatus(200);
            response.setHeader("Content-Type", getMimeType(filepath));
            response.setHeader("Content-Length", intToString(content.size()));
            response.setBody(content);
            
            logMessage("Serving " + filepath + " (" + intToString(content.size()) + " bytes)");
        } else {
            response.setStatus(500);
            response.setHeader("Content-Type", "text/html");
            response.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
        }
    } else {
        logMessage("File not found: " + filepath);
        response.setStatus(404);
        response.setHeader("Content-Type", "text/html");
        
        // Use custom error page if configured
        std::map<int, std::string>::const_iterator it = _config.errorPages.find(404);
        if (it != _config.errorPages.end()) {
            std::string errorPagePath = _config.root + it->second;
            if (fileExists(errorPagePath)) {
                std::string errorContent = readFile(errorPagePath);
                response.setBody(errorContent);
                return;
            }
        }
        
        // Default 404 page
        response.setBody("<html><body><h1>404 Not Found</h1><p>The requested file was not found.</p></body></html>");
    }
}

// ============================================================================
// HANDLE POST REQUEST
// ============================================================================

void Server::_handlePOST(const Request& request, Response& response) {
    // Check if there's content
    if (request.getContentLength() == 0) {
        response.setStatus(400);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>400 Bad Request</h1><p>No content provided.</p></body></html>");
        return;
    }
    
    const LocationConfig* location = _findLocation(request.getUri());
    size_t maxBodySize = _config.clientMaxBodySize;     //server default

    if (location && location->clientMaxBodySize > 0)
    {
        maxBodySize = location->clientMaxBodySize;
    }

    if (request.getContentLength() > maxBodySize)
    {
        response.setStatus(413);
        response.setBody("<html><body><h1>413 Payload Too Large</h1></body></html>");
        logError("Request body too large: " + intToString(request.getContentLength()) + 
                " > " + intToString(maxBodySize));
        return;
    }
    
    // Handle multipart file upload
    if (request.isMultipartUpload()) {
        std::string fileName = request.getUploadedFileName();
        std::string fileContent = request.getUploadedFileContent();
        
        // Validate filename
        if (!isPathSafe(fileName) || fileName.empty()) {
            logError("Unsafe or empty filename: " + fileName);
            response.setStatus(400);
            response.setHeader("Content-Type", "text/html");
            response.setBody("<html><body><h1>400 Bad Request</h1><p>Invalid filename.</p></body></html>");
            return;
        }
        
        // Save file
        std::string uploadPath = "./www/uploads/" + fileName;
        
        if (writeFile(uploadPath, fileContent)) {
            response.setStatus(201);
            response.setHeader("Content-Type", "text/plain");
            response.setBody("File uploaded successfully: " + fileName);
            logMessage("File uploaded: " + uploadPath + " (" + intToString(fileContent.size()) + " bytes)");
        } else {
            response.setStatus(500);
            response.setHeader("Content-Type", "text/html");
            response.setBody("<html><body><h1>500 Internal Server Error</h1><p>Failed to save file.</p></body></html>");
        }
    } else {
        response.setStatus(400);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>400 Bad Request</h1><p>Unsupported content type for POST.</p></body></html>");
    }
}

// ============================================================================
// HANDLE DELETE REQUEST
// ============================================================================

void Server::_handleDELETE(const Request& request, Response& response) {
    std::string uri = request.getUri();
    
    // Validate path
    if (!isPathSafe(uri)) {
        logError("Unsafe path in DELETE request: " + uri);
        response.setStatus(403);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>403 Forbidden</h1><p>Invalid file path.</p></body></html>");
        return;
    }
    
    std::string filepath = "./uploads" + uri;
    logMessage("DELETE request for: " + filepath);
    
    // Check if file exists
    if (fileExists(filepath)) {
        if (deleteFile(filepath)) {
            response.setStatus(204);  // No Content
            logMessage("File deleted successfully: " + filepath);
        } else {
            response.setStatus(500);
            response.setHeader("Content-Type", "text/html");
            response.setBody("<html><body><h1>500 Internal Server Error</h1><p>Failed to delete file.</p></body></html>");
        }
    } else {
        logMessage("Cannot delete - file not found: " + filepath);
        response.setStatus(404);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>404 Not Found</h1><p>File does not exist.</p></body></html>");
    }
}

void Server::_processRequest(Client* client)
{
    Request request;
    Response response;
    
    // Parse the request
    if (!request.parse(client->getRequest())) {
        response.setStatus(400);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>400 Bad Request</h1></body></html>");
        client->setResponse(response.toString());
        return;
    }
    
    std::string method = request.getMethod();
    std::string uri = request.getUri();

    logMessage("Method: " + method + ", URI: " + uri);
    
    if (method != "GET" && method != "POST" && method != "DELETE")
    {
        _setErrorResponse(response, 501);
        client->setResponse(response.toString());
        return;
    }

    const LocationConfig* loc = _findLocation(uri);
    if (loc)
    {
        bool methodAllowed = false;
        for (size_t i = 0; i < loc->allowedMethods.size(); i++)
        {
            if (loc->allowedMethods[i] == method)
            {
                methodAllowed = true;
                break ;
            }
        }
        if (!methodAllowed) {
            _setErrorResponse(response, 405);
            client->setResponse(response.toString());
            return ;
        }
    }

    if (method == "GET")
        _handleGET(request, response);
    else if (method == "POST")
        _handlePOST(request, response);
    else if (method == "DELETE")
        _handleDELETE(request, response);
    client->setResponse(response.toString());
}

void Server::_readRequest(int fd, Client* client) {
    char buffer[4096];
    ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        client->appendToRequest(std::string(buffer, bytesRead));
        
        if (client->isRequestComplete()) {
            logMessage("Request received from socket " + intToString(fd));
            _processRequest(client);
            
            // Switch to sending mode
            for (size_t i = 0; i < _pollFds.size(); ++i) {
                if (_pollFds[i].fd == fd) {
                    _pollFds[i].events = POLLOUT;
                    break;
                }
            }
        }
    } else if (bytesRead == 0) {
        logMessage("Client disconnected: socket " + intToString(fd));
        _closeConnection(fd);
    } else {
        logError("Error reading from socket " + intToString(fd));
        _closeConnection(fd);
    }
}

void    Server::_sendResponse(int fd, Client* client)
{
    std::string chunk = client->getResponseChunk(4096);
    
    if (!chunk.empty()) {
        ssize_t bytesSent = send(fd, chunk.c_str(), chunk.size(), 0);
        
        if (bytesSent < 0) {
            logError("Error sending to socket " + intToString(fd));
            _closeConnection(fd);
            return;
        }
    }
    
    // Check if we're done sending
    if (!client->hasMoreToSend()) {
        logMessage("Response sent to socket " + intToString(fd));
        _closeConnection(fd);
    }
}

void Server::_handleClient(int fd) {
    Client* client = _clients[fd];
    if (!client) {
        return;
    }
    
    if (client->getState() == READING_REQUEST) {
        _readRequest(fd, client);
    } else if (client->getState() == SENDING_RESPONSE) {
        _sendResponse(fd, client);
    }
}

// Close connection
void Server::_closeConnection(int fd) {
    // Remove from clients map
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it != _clients.end()) {
        delete it->second;
        _clients.erase(it);
    }
    
    // Remove from poll array
    _removeFromPoll(fd);
    
    // Close socket
    close(fd);
}

// Remove from poll array
void Server::_removeFromPoll(int fd) {
    for (std::vector<struct pollfd>::iterator it = _pollFds.begin();
         it != _pollFds.end(); ++it) {
        if (it->fd == fd) {
            _pollFds.erase(it);
            break;
        }
    }
}

// Main server loop
void Server::run() {
    logMessage("Server is running... Press Ctrl+C to stop");
    
    while (true) {
        // Call poll with timeout
        int pollCount = poll(&_pollFds[0], _pollFds.size(), TIMEOUT);
        
        if (pollCount < 0) {
            logError("Poll error");
            break;
        }
        
        if (pollCount == 0) {
            // Timeout - could check for inactive clients here
            continue;
        }
        
        // Check which file descriptors are ready
        for (size_t i = 0; i < _pollFds.size(); ++i) {
            if (_pollFds[i].revents == 0) {
                continue;  // No events on this fd
            }
            
            // Check for errors
            if (_pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                if (_pollFds[i].fd != _serverSocket) {
                    logMessage("Connection closed on socket " + intToString(_pollFds[i].fd));
                    _closeConnection(_pollFds[i].fd);
                }
                continue;
            }
            
            // Server socket - new connection
            if (_pollFds[i].fd == _serverSocket) {
                if (_pollFds[i].revents & POLLIN) {
                    _acceptNewConnection();
                }
            }
            // Client socket - read or write
            else {
                if (_pollFds[i].revents & POLLIN) {
                    _handleClient(_pollFds[i].fd);
                } else if (_pollFds[i].revents & POLLOUT) {
                    _handleClient(_pollFds[i].fd);
                }
            }
        }
    }
}

// Getters
int Server::getPort() const {
    return _config.port;
}

const LocationConfig* Server::_findLocation(const std::string& uri) const {
    const LocationConfig* bestMatch = NULL;
    size_t longestMatch = 0;
    
    for (size_t i = 0; i < _config.locations.size(); i++) {
        const LocationConfig& loc = _config.locations[i];
        
        // Check if URI starts with location path
        if (uri.find(loc.path) == 0) {
            size_t matchLen = loc.path.length();
            if (matchLen > longestMatch) {
                longestMatch = matchLen;
                bestMatch = &loc;
            }
        }
    }
    
    return bestMatch;
}

void Server::_setErrorResponse(Response& response, int statusCode)
{
    response.setStatus(statusCode);
    response.setHeader("Content-Type", "text/html");
    
    std::map<int, std::string>::const_iterator it = _config.errorPages.find(statusCode);
    if (it != _config.errorPages.end()) {
        std::string errorPagePath = _config.root + it->second;
        if (fileExists(errorPagePath)) {
            std::string errorContent = readFile(errorPagePath);
            if (!errorContent.empty()) {
                response.setBody(errorContent);
                return;
            }
        }
    }
    
    // Default error pages
    std::string defaultBody;
    switch (statusCode) {
        case 400: defaultBody = "<html><body><h1>400 Bad Request</h1></body></html>"; break;
        case 403: defaultBody = "<html><body><h1>403 Forbidden</h1></body></html>"; break;
        case 404: defaultBody = "<html><body><h1>404 Not Found</h1></body></html>"; break;
        case 405: defaultBody = "<html><body><h1>405 Method Not Allowed</h1></body></html>"; break;
        case 413: defaultBody = "<html><body><h1>413 Payload Too Large</h1></body></html>"; break;
        case 500: defaultBody = "<html><body><h1>500 Internal Server Error</h1></body></html>"; break;
        case 501: defaultBody = "<html><body><h1>501 Not Implemented</h1></body></html>"; break;
        default:  defaultBody = "<html><body><h1>Error</h1></body></html>"; break;
    }
    response.setBody(defaultBody);
}
