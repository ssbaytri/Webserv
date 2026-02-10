#include <iostream>
#include <cstdlib>
#include "../includes/Server.hpp"
#include "../includes/utils.hpp"

int main(int argc, char **argv)
{
	int port = 8080;

	if (argc == 2)
	{
		port = std::atoi(argv[1]);
		if (port <= 0 || port > 65535)
		{
			logError("Invalid port number");
			return (1);
		}
	}

	try {
        logMessage("Starting webserv on port " + intToString(port));
        Server server(port);
        server.run();
    } catch (const std::exception& e) {
        logError(std::string("Fatal error: ") + e.what());
        return 1;
    }

	return (0);
}