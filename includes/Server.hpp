#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <map>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include "Client.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"

#define TIMEOUT 5000

class Server
{
	private:
		std::vector<ServerConfig>		_configs;
		std::map<int, ServerConfig*>	_socketToConfig;
		std::vector<int>				_serverSockets;
		std::vector<struct pollfd>		_pollFds;
		std::map<int, Client*>			_clients;

		void    _setupSockets();
		int		_createListeningSocket(int port);
		void    _setNonBlocking(int fd);
		void    _acceptNewConnection(int serverSocket);
		void    _handleClient(int fd);
		void    _closeConnection(int fd);
		void    _removeFromPoll(int fd);

		void	_readRequest(int fd, Client* client);
		void	_processRequest(Client* client, const ServerConfig& config);
		void	_handleGET(const Request& request, Response& response);
		void	_handlePOST(const Request& request, Response& response);
		void	_handleDELETE(const Request& request, Response& response);
		void	_sendResponse(int fd, Client* client);

		void	_setErrorResponse(Response& response, int statusCode);
		const LocationConfig* _findLocation(const std::string& uri) const;
		std::string _generateDirectoryListing(const std::string& dirPath, const std::string& uri);

	public:
		Server(const std::vector<ServerConfig>& configs);
		~Server();
		void run();
};

#endif