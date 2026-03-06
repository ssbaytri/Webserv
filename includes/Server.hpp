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
		ServerConfig				_config;
		int							_serverSocket;
		std::vector<struct pollfd>	_pollFds;
		std::map<int, Client*>		_clients;

		void    _setupSocket();
		void    _setNonBlocking(int fd);
		void    _acceptNewConnection();
		void    _handleClient(int fd);
		void    _closeConnection(int fd);
		void    _removeFromPoll(int fd);

		void	_readRequest(int fd, Client* client);
		void	_processRequest(Client* client);
		void	_handleGET(const Request& request, Response& response);
		void	_handlePOST(const Request& request, Response& response);
		void	_handleDELETE(const Request& request, Response& response);
		void	_sendResponse(int fd, Client* client);

		void	_setErrorResponse(Response& response, int statusCode);

		const LocationConfig* _findLocation(const std::string& uri) const;

	public:
		Server(const ServerConfig& config);
		~Server();

		void run();

		int getPort() const;
};

#endif