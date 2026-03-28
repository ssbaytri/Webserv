#include "../includes/Server.hpp"
#include "../includes/utils.hpp"
#include "../includes/Request.hpp"
#include "../includes/Response.hpp"
#include <arpa/inet.h>
#include <stdexcept>
#include <fstream>
#include <sys/wait.h>
#include <dirent.h>

volatile bool g_shutdown = false;

void signalHandler(int signal)
{
    (void)signal;
    g_shutdown = true;
    logMessage("Received shutdown signal, closing connections...");
}

// Constructor
Server::Server(const std::vector<ServerConfig>& configs) : _configs(configs) {
    if (_configs.empty())
        throw std::runtime_error("No server configurations provided");

    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // kill command
    logMessage("Signal handlers registered");

    _setupSockets();
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
    
    // clear all server sockets
    for (size_t i = 0; i < _serverSockets.size(); i++)
        close(_serverSockets[i]);
    
    logMessage("Server shut down");
}

int Server::_createListeningSocket(int port)
{
    struct sockaddr_in serverAddr;

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        logError("Failed to create socket for port " + intToString(port));
        return (-1);
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverSocket);
        logError("Failed to set socket options for port " + intToString(port));
        return -1;
    }

    _setNonBlocking(serverSocket);

    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        close(serverSocket);
        logError("Failed to bind socket to port " + intToString(port));
        return (-1);
    }

    if (listen(serverSocket, 128) < 0)
    {
        close(serverSocket);
        logError("Failed to listen on port " + intToString(port));
        return (-1);
    }
    return (serverSocket);
}

void Server::_setupSockets()
{
    for (size_t i = 0; i < _configs.size(); i++)
    {
        int port = _configs[i].port;

        int serverSocket = _createListeningSocket(port);
        if (serverSocket < 0)
            throw std::runtime_error("Failed to create socket for port " + intToString(port));

        _serverSockets.push_back(serverSocket);

        _socketToConfig[serverSocket]  = &_configs[i];

        // Add to poll
        struct pollfd serverPollFd;
        serverPollFd.fd = serverSocket;
        serverPollFd.events = POLLIN;
        serverPollFd.revents = 0;
        _pollFds.push_back(serverPollFd);

        logMessage("Server listening on port " + intToString(port));
    }
}


void Server::_setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) 
        throw std::runtime_error("Failed to get socket flags");
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) 
        throw std::runtime_error("Failed to set non-blocking mode");
}


void Server::_acceptNewConnection(int serverSocket)
{
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSocket < 0) return;

    _setNonBlocking(clientSocket);

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

    ServerConfig* config = _socketToConfig[serverSocket];
    
    logMessage("New connection from " + std::string(clientIP) + 
               " on port " + intToString(config->port) +
               " (socket " + intToString(clientSocket) + ")");

    Client* newClient = new Client(clientSocket);
    _clients[clientSocket] = newClient;

    _socketToConfig[clientSocket] = config;

    struct pollfd clientPollFd;
    clientPollFd.fd = clientSocket;
    clientPollFd.events = POLLIN;
    clientPollFd.revents = 0;
    _pollFds.push_back(clientPollFd);
}

std::string Server::_generateDirectoryListing(const std::string& dirPath, const std::string& uri) {
    std::vector<FileInfo> files = listDirectory(dirPath);
    
    std::string html = "<!DOCTYPE html>\n<html>\n<head>\n";
    html += "<meta charset=\"UTF-8\">\n";
    html += "<title>Index of " + uri + "</title>\n";
    html += "<style>\n";
    html += "body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }\n";
    html += ".container { background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); max-width: 900px; margin: 0 auto; }\n";
    html += "h1 { color: #333; border-bottom: 2px solid #0066cc; padding-bottom: 10px; }\n";
    html += "table { width: 100%; border-collapse: collapse; margin-top: 20px; }\n";
    html += "th { text-align: left; padding: 10px; background: #f0f0f0; border-bottom: 2px solid #ddd; }\n";
    html += "td { padding: 10px; border-bottom: 1px solid #eee; }\n";
    html += "tr:hover { background: #f9f9f9; }\n";
    html += "a { color: #0066cc; text-decoration: none; }\n";
    html += "a:hover { text-decoration: underline; }\n";
    html += ".directory { font-weight: bold; color: #ff9800; }\n";
    html += ".directory:before { content: '📁 '; }\n";
    html += ".file:before { content: '📄 '; }\n";
    html += ".parent:before { content: '⬆️ '; }\n";
    html += ".size { color: #999; text-align: right; }\n";
    html += ".date { color: #666; text-align: right; }\n";
    html += ".empty { color: #999; font-style: italic; text-align: center; padding: 40px; }\n";
    html += "footer { margin-top: 30px; text-align: center; color: #999; font-size: 0.9em; border-top: 1px solid #ddd; padding-top: 20px; }\n";
    html += "</style>\n";
    html += "</head>\n<body>\n";
    html += "<div class=\"container\">\n";
    html += "<h1>Index of " + uri + "</h1>\n";
    
    // Check if directory listing is empty
    if (files.empty()) {
        html += "<p class=\"empty\">Directory is empty</p>\n";
    } else {
        html += "<table>\n";
        html += "<thead>\n<tr>\n";
        html += "<th>Name</th>\n";
        html += "<th class=\"size\">Size</th>\n";
        html += "<th class=\"date\">Modified</th>\n";
        html += "</tr>\n</thead>\n<tbody>\n";
        
        // Add parent directory link (unless at root)
        if (uri != "/") {
            html += "<tr>\n";
            html += "<td><a href=\"../\" class=\"parent\">Parent Directory</a></td>\n";
            html += "<td class=\"size\">-</td>\n";
            html += "<td class=\"date\">-</td>\n";
            html += "</tr>\n";
        }
        
        // Add each file/directory
        for (size_t i = 0; i < files.size(); i++) {
            const FileInfo& file = files[i];
            
            std::string className = file.isDirectory ? "directory" : "file";
            html += "<tr>\n";
            html += "<td><a href=\"" + file.name + "\" class=\"" + className + "\">" + file.name + "</a></td>\n";
            
            if (file.isDirectory) {
                html += "<td class=\"size\">-</td>\n";
            } else {
                html += "<td class=\"size\">" + formatSize(file.size) + "</td>\n";
            }
            
            html += "<td class=\"date\">" + formatTime(file.modified) + "</td>\n";
            html += "</tr>\n";
        }
        
        html += "</tbody>\n</table>\n";
    }
    
    html += "<footer>webserv - HTTP Server</footer>\n";
    html += "</div>\n";
    html += "</body>\n</html>";
    
    return html;
}

void Server::_handleGET(const Request& request, Response& response, const ServerConfig& config)
{
    std::string uri = request.getUri();
    
    std::string root = config.root;
    const LocationConfig* location = _findLocation(uri, config);

    if (location && !location->root.empty())
        root = location->root;

    std::string filepath = root + uri;

    if (location && !location->cgiPass.empty())
    {
        std::string extension = getFileExtension(filepath);
        
        std::map<std::string, std::string>::const_iterator it = location->cgiPass.find(extension);
        if (it != location->cgiPass.end())
        {
            _handleCGI(request, response, filepath, it->second, config);
            return ;
        }
    }

    if (uri == "/" || uri[uri.length() - 1] == '/')
    {
        std::vector<std::string> indexFiles;

        if (location && !location->index.empty())
            indexFiles = location->index;
        else if (!config.index.empty())
            indexFiles = config.index;
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
            if (!isDirectory(filepath))
            {
                _setErrorResponse(response, 404, config);
                return ;
            }

            if (location && location->autoindex)
            {
                DIR* testDir = opendir(filepath.c_str());
                if (!testDir)
                {
                    logError("Permission denied for directory: " + filepath);
                    _setErrorResponse(response, 403, config);
                    return;
                }
                closedir(testDir);

                std::string html = _generateDirectoryListing(filepath, uri);

                response.setStatus(200);
                response.setHeader("Content-Type", "text/html; charset=UTF-8");
                response.setHeader("Content-Length", intToString(html.size()));
                response.setBody(html);

                logMessage("Generated directory listing for: " + filepath);
                return ;
            }

            _setErrorResponse(response, 403, config);
            return ;
        }
    }
    
    logMessage("Serving file: " + filepath);
    
    // Check if file exists
    if (fileExists(filepath))
    {
        std::string content = readFile(filepath);
        
        if (!content.empty()) {
            response.setStatus(200);
            response.setHeader("Content-Type", getMimeType(filepath));
            response.setHeader("Content-Length", intToString(content.size()));
            response.setBody(content);
            
            logMessage("Serving " + filepath + " (" + intToString(content.size()) + " bytes)");
        } else {
            logError("Failed to read file: " + filepath);
            _setErrorResponse(response, 500, config);
        }
    }
    else
    {
        logMessage("File not found: " + filepath);
        _setErrorResponse(response, 404, config);
    }
}

void Server::_handlePOST(const Request& request, Response& response, const ServerConfig& config){
    bool isChunked = request.getTransferEncoding() == "chunked";

    if (request.getContentLength() == 0 && !isChunked)
    {
        _setErrorResponse(response, 400, config);
        return ;
    }
    
    const LocationConfig* location = _findLocation(request.getUri(), config);

    if (location && !location->cgiPass.empty())
    {
        std::string root = config.root;
        if (location && !location->root.empty())
            root = location->root;

        std::string file_path = root + request.getUri();
        std::string extension = getFileExtension(file_path);
        std::map<std::string, std::string>::const_iterator it = location->cgiPass.find(extension);
        if (it != location->cgiPass.end())
        {
            _handleCGI(request, response, file_path, it->second, config);
            return ;
        }
    }
    
    // Body size check
    size_t maxBodySize = config.clientMaxBodySize;
    if (location && location->clientMaxBodySize > 0) {
        maxBodySize = location->clientMaxBodySize;
    }
    
    if (request.getContentLength() > maxBodySize) {
        _setErrorResponse(response, 413, config);
        return;
    }
    
    if (request.isMultipartUpload()) {
        std::string fileName = request.getUploadedFileName();
        std::string fileContent = request.getUploadedFileContent();
        
        if (!isPathSafe(fileName) || fileName.empty()) {
            _setErrorResponse(response, 400, config);
            return;
        }
        
        // === USE ROOT (same as GET/DELETE) ===
        std::string root = config.root;
        if (location && !location->root.empty()) {
            root = location->root;
        }
        
        // Determine upload directory from URI
        std::string uri = request.getUri();
        std::string uploadDir = root + uri;
        
        // Ensure trailing slash
        if (!uploadDir.empty() && uploadDir[uploadDir.length() - 1] != '/') {
            uploadDir += "/";
        }
        
        std::string filepath = uploadDir + fileName;
        
        if (writeFile(filepath, fileContent)) {
            response.setStatus(201);
            response.setBody("File uploaded successfully: " + fileName);
            logMessage("File uploaded: " + filepath);
        } else {
            _setErrorResponse(response, 500, config);
        }
    } else {
        _setErrorResponse(response, 400, config);
    }
}

void Server::_handleDELETE(const Request& request, Response& response, const ServerConfig& config){
    std::string uri = request.getUri();
    
    if (!isPathSafe(uri)) {
        _setErrorResponse(response, 403, config);
        return;
    }
    
    // === USE ROOT (same as GET/POST) ===
    std::string root = config.root;
    const LocationConfig* location = _findLocation(uri, config);
    
    if (location && !location->root.empty()) {
        root = location->root;
    }
    
    std::string filepath = root + uri;
    
    logMessage("DELETE request for: " + filepath);
    
    if (fileExists(filepath)) {
        if (deleteFile(filepath)) {
            response.setStatus(204);
            logMessage("File deleted successfully: " + filepath);
        } else {
            _setErrorResponse(response, 500, config);
        }
    } else {
        _setErrorResponse(response, 404, config);
    }
}

void Server::_processRequest(Client* client, const ServerConfig& config)
{
    Request request;
    Response response;
    
    // Parse the request
    if (!request.parse(client->getRequest())) {
        _setErrorResponse(response, 400, config);
        response.setHeader("Connection", "close");
        client->setShouldClose(true);
        client->setResponse(response.toString());
        return;
    }
    
    std::string method = request.getMethod();
    std::string uri = request.getUri();
    std::string conn = request.getConnection();

    if (conn == "close")
        client->setShouldClose(true);
    else
        client->setShouldClose(false);

    response.setHeader("Connection", client->shouldClose() ? "close" : "keep-alive");

    logMessage("Method: " + method + ", URI: " + uri);
    
    if (method != "GET" && method != "POST" && method != "DELETE")
    {
        _setErrorResponse(response, 501, config);
        client->setResponse(response.toString());
        return;
    }

    const LocationConfig* loc = _findLocation(uri, config);

    if (loc && !loc->redirect.empty())
    {
        response.setStatus(301);
        response.setHeader("Location", loc->redirect);
        response.setHeader("Content-Length", "0");
        response.setBody("");
        logMessage("Redirecting " + uri + " → " + loc->redirect);
        client->setResponse(response.toString());
        return ;
    }

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
            _setErrorResponse(response, 405, config);
            client->setResponse(response.toString());
            return ;
        }
    }

    if (method == "GET")
        _handleGET(request, response, config);
    else if (method == "POST")
        _handlePOST(request, response, config);
    else if (method == "DELETE")
        _handleDELETE(request, response, config);
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

            ServerConfig* config = _socketToConfig[fd];
            _processRequest(client, *config);
            
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
    if (!client->hasMoreToSend())
    {
        logMessage("Response sent to socket " + intToString(fd));

        if (client->shouldClose())
            _closeConnection(fd);
        else
        {
            client->clearRequest();
            client->setState(READING_REQUEST);

            for (size_t i = 0; i < _pollFds.size(); i++)
            {
                if (_pollFds[i].fd == fd)
                {
                    _pollFds[i].events = POLLIN;
                    break;
                }
            }
            logMessage("Connection kept alive on socket " + intToString(fd));
        }
    }
}

void Server::_handleClient(int fd)
{
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

void Server::_closeConnection(int fd)
{
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it != _clients.end()) {
        delete it->second;
        _clients.erase(it);
    }
    
    _removeFromPoll(fd);
    close(fd);
}

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
    logMessage("Server is running on " + intToString(_configs.size()) + " port(s)");

    while (!g_shutdown)
    {
        int pollCount = poll(&_pollFds[0], _pollFds.size(), TIMEOUT);

        if (pollCount < 0)
        {
            if (errno == EINTR)
            {
                logMessage("Poll interrupted by signal");
                continue;
            }
            logError("Poll Error");
            break;
        }

        std::vector<int> timedOutSockets;
        for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        {
            if (it->second->isTimedOut(IDLE_TIMEOUT))
                timedOutSockets.push_back(it->first);
        }

        for (size_t i = 0; i < timedOutSockets.size(); ++i)
        {
            logMessage("Client idle timeout on socket " + intToString(timedOutSockets[i]));
            _closeConnection(timedOutSockets[i]);
        }

        if (pollCount == 0)
            continue;

        for (size_t i = 0; i < _pollFds.size(); i++)
        {
            if (_pollFds[i].revents == 0)
                continue;
            
            if (_pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                bool isServerSocket = false;
                for (size_t j = 0; j < _serverSockets.size(); j++)
                {
                    if (_pollFds[i].fd == _serverSockets[j])
                    {
                        isServerSocket = true;
                        break ;
                    }
                }

                if (!isServerSocket)
                {
                    logMessage("Connection closed on socket " + intToString(_pollFds[i].fd));
                    _closeConnection(_pollFds[i].fd);
                }
                continue;
            }

            bool isServerSocket = false;
            for (size_t j = 0; j < _serverSockets.size(); j++)
            {
                if (_pollFds[i].fd == _serverSockets[j])
                {
                    if (_pollFds[i].revents & POLLIN)
                        _acceptNewConnection(_serverSockets[j]);
                    isServerSocket = true;
                    break ;
                }
            }

            if (!isServerSocket)
            {
                if (_pollFds[i].revents & POLLIN)
                    _handleClient(_pollFds[i].fd);
                else if (_pollFds[i].revents & POLLOUT)
                    _handleClient(_pollFds[i].fd);
            }
        }
    }

    logMessage("Shutting down gracefully...");
    
    std::vector<int> clientSockets;
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        clientSockets.push_back(it->first);

    for (size_t i = 0; i < clientSockets.size(); ++i)
    {
        logMessage("Closing client socket " + intToString(clientSockets[i]));
        _closeConnection(clientSockets[i]);
    }
    
    logMessage("All connections closed");
}

const LocationConfig* Server::_findLocation(const std::string& uri, const ServerConfig& config) const
{
    const LocationConfig* bestMatch = NULL;
    size_t longestMatch = 0;

    for (size_t i = 0; i < config.locations.size(); i++)
    {
        const LocationConfig& loc = config.locations[i];
        const std::string& path = loc.path;
        size_t locLen = path.length();

        if (locLen == 0)
            continue;

        // Must start with the location path
        if (uri.compare(0, locLen, path) != 0)
            continue;

        // Boundary condition:
        // - exact match:  /uploads  matches location /uploads
        // - prefix match: /uploads/anything matches location /uploads
        // - but /uploadsX should NOT match /uploads
        bool boundaryOk = (uri.length() == locLen) ||
                          (uri.length() > locLen && uri[locLen] == '/');

        if (!boundaryOk)
            continue;

        if (locLen > longestMatch)
        {
            longestMatch = locLen;
            bestMatch = &loc;
        }
    }

    return bestMatch;
}

void Server::_setErrorResponse(Response& response, int statusCode, const ServerConfig& config)
{
    response.setStatus(statusCode);
    response.setHeader("Content-Type", "text/html");
    
    std::map<int, std::string>::const_iterator it = config.errorPages.find(statusCode);
    if (it != config.errorPages.end()) {
        std::string errorPagePath = config.root + it->second;
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

void Server::_handleCGI(const Request& request, Response& response, 
                const std::string& scriptPath, const std::string& cgiExecutor,
                const ServerConfig& config)
{
    CgiHandler cgi(request, scriptPath);

    if (cgi.setupIO(request.getBody()) < 0)
    {
        _setErrorResponse(response, 500, config);
        return ;
    }

    if (cgi.executeCgi(scriptPath, cgiExecutor) < 0)
    {
        _setErrorResponse(response, 500, config);
        return ;
    }

    std::string cgiOutput;
    char buffer[4096];
    time_t startTime = std::time(NULL);
    int cgiTimeout = 5;
    
    while (true)
    {
        if (std::time(NULL) - startTime >= cgiTimeout)
        {
            logError("CGI timeout — killing process");
            kill(cgi.getPid(), SIGKILL);
            waitpid(cgi.getPid(), NULL, 0);
            _setErrorResponse(response, 504, config);
            return;
        }

        struct pollfd pfd;
        pfd.fd = cgi.getOutputFd();
        pfd.events = POLLIN;
        pfd.revents = 0;

        int ready = poll(&pfd, 1, 1000);

        if (ready < 0) {
            _setErrorResponse(response, 500, config);
            return;
        }

        if (ready == 0)
            continue;

        if (pfd.revents & POLLHUP && !(pfd.revents & POLLIN))
            break;

        ssize_t bytesRead = read(cgi.getOutputFd(), buffer, sizeof(buffer));
        if (bytesRead <= 0)
            break;
        cgiOutput += std::string(buffer, bytesRead);
    }

    int status;
    waitpid(cgi.getPid(), &status, 0);

    if (cgiOutput.empty())
    {
        _setErrorResponse(response, 500, config);
        return;
    }

    size_t headerEnd = cgiOutput.find("\r\n\r\n");
    size_t separatorLen = 4;

    if (headerEnd == std::string::npos) {
        headerEnd = cgiOutput.find("\n\n");
        separatorLen = 2;
    }
    
    if (headerEnd == std::string::npos) {
        _setErrorResponse(response, 500, config);
        return;
    }

    std::string cgiHeaders = cgiOutput.substr(0, headerEnd);
    std::string cgiBody = cgiOutput.substr(headerEnd + separatorLen);

    response.setStatus(200);
    response.setBody(cgiBody);

    std::istringstream stream(cgiHeaders);
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.empty() || line == "\r") continue;
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = trim(line.substr(0, colon));
        std::string val = trim(line.substr(colon + 1));
        if (key == "Status")
            response.setStatus(atoi(val.c_str()));
        else
            response.setHeader(key, val);
    }
}