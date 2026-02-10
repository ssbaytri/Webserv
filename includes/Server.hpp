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

#define TIMEOUT 5000

class Server
{
	private:
		int							_port;
		int							_serverSocket;
		std::vector<struct pollfd>	_pollFds;
		std::map<int, Client*>		_clients;

		void    _setupSocket();
		void    _setNonBlocking(int fd);
		void    _acceptNewConnection();
		void    _handleClient(int fd);
		void    _closeConnection(int fd);
		void    _removeFromPoll(int fd);

	public:
		Server(int port);
		~Server();

		void run();

		int getPort() const;
};

#endif