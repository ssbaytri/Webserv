#include "../includes/Server.hpp"
#include "../includes/utils.hpp"
#include "../includes/Request.hpp"
#include "../includes/Response.hpp"
#include <arpa/inet.h>
#include <stdexcept>
#include <fstream>

// Constructor
Server::Server(int port) : _port(port), _serverSocket(-1) {
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
    serverAddr.sin_port = htons(_port);
    
    // 5. Bind socket to address
    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(_serverSocket);
        throw std::runtime_error("Failed to bind socket to port " + intToString(_port));
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
    
    logMessage("Server listening on port " + intToString(_port));
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

// Handle client communication
void Server::_handleClient(int fd) {
    Client* client = _clients[fd];
    if (!client) {
        return;
    }
    
    if (client->getState() == READING_REQUEST) {
        // Read data from client
        char buffer[4096];
        ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            client->appendToRequest(std::string(buffer, bytesRead));
            
            // Check if request is complete
            if (client->isRequestComplete()) {
                logMessage("Request received from socket " + intToString(fd));

                std::string requestRaw = client->getRequest();

                Request request;
                Response response;

                if (!request.parse(requestRaw))
                {
                    response.setStatus(400);
                    response.setHeader("Content-Type", "text/html");
                    response.setBody("<html><body><h1>400 Bad Request</h1></body></html>");
                }
                else
                {
                    std::string method = request.getMethod();
                    std::string uri = request.getUri();

                    logMessage("Method: " + method + ", URI: " + uri);

                    if (method == "GET")
                    {
                        std::string file_path = "./www" + uri;
                        if (uri == "/") file_path = "./www/index.html";

                        logMessage("Serving file: " + file_path);

                        if (fileExists(file_path))
                        {
                            std::string content = readFile(file_path);

                            if (!content.empty())
                            {
                                response.setStatus(200);
                                response.setHeader("Content-Type", getMimeType(file_path));
                                response.setHeader("Content-Length", intToString(content.size()));
                                response.setBody(content);

                                logMessage("Serving " + file_path + " (" + intToString(content.size()) + " bytes)");
                            }
                            else
                            {
                                response.setStatus(500);
                                response.setHeader("Content-Type", "text/html");
                                response.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
                            }
                        }
                        else
                        {
                            logMessage("File not found: " + file_path);
                            response.setStatus(404);
                            response.setHeader("Content-Type", "text/html");
                            response.setBody("<html><body><h1>404 Not Found</h1><p>The requested file was not found.</p></body></html>");
                        }
                    }
                    else if (method == "DELETE")
                    {
                        if (!isPathSafe(uri))
                        {
                            logError("Unsafe path in DELETE request: " + uri);
                            response.setStatus(403);
                            response.setHeader("Content-Type", "text/html");
                            response.setBody("<html><body><h1>403 Forbidden</h1><p>Invalid file path.</p></body></html>");
                        }
                        else
                        {
                            std::string file_path = "./www" + uri;
                            logMessage("DELETE request for: " + file_path);

                            if (fileExists(file_path))
                            {
                                if (deleteFile(file_path))
                                {
                                    response.setStatus(204);
                                    logMessage("File deleted successfully: " + file_path);
                                }
                                else
                                {
                                    response.setStatus(500);
                                    response.setHeader("Content-Type", "text/html");
                                    response.setBody("<html><body><h1>500 Internal Server Error</h1><p>Failed to delete file.</p></body></html>");
                                }
                            }
                            else
                            {
                                logMessage("Cannot delete - file not found: " + file_path);
                                response.setStatus(404);
                                response.setHeader("Content-Type", "text/html");
                                response.setBody("<html><body><h1>404 Not Found</h1><p>File does not exist.</p></body></html>");
                            }
                        }
                    }
                    else
                    {
                        response.setStatus(501);
                        response.setHeader("Content-Type", "text/html");
                        response.setBody("<html><body><h1>501 Not Implemented</h1><p>Method " + method + " is not supported yet.</p></body></html>");
                    }
                }
                
                response.setHeader("Connection", "close");

                client->setResponse(response.toString());

                // Update poll to monitor for writing
                for (size_t i = 0; i < _pollFds.size(); ++i) {
                    if (_pollFds[i].fd == fd) {
                        _pollFds[i].events = POLLOUT;
                        break;
                    }
                }
            }
        } else if (bytesRead == 0) {
            // Client closed connection
            logMessage("Client disconnected: socket " + intToString(fd));
            _closeConnection(fd);
        } else {
            logError("Error reading from socket " + intToString(fd));
            _closeConnection(fd);
        }
    } else if (client->getState() == SENDING_RESPONSE) {
        // Send data to client
        std::string chunk = client->getResponseChunk(4096);
        
        if (!chunk.empty()) {
            ssize_t bytesSent = send(fd, chunk.c_str(), chunk.size(), 0);
            
            if (bytesSent < 0) {
                logError("Error sending to socket " + intToString(fd));
                _closeConnection(fd);
                return;
            }
            
            // If we sent less than the chunk, we need to adjust
            // For now, we'll keep it simple and send full chunks
        }
        
        // Check if we're done sending
        if (!client->hasMoreToSend()) {
            logMessage("Response sent to socket " + intToString(fd));
            _closeConnection(fd);  // Close connection after response
        }
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
    return _port;
}