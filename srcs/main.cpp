#include <iostream>
#include <cstdlib>
#include "../includes/Server.hpp"
#include "../includes/utils.hpp"
#include "../includes/Config.hpp"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        logError("Error: invalid arguments");
        logError("Usage: ./webserv <config_file>");
        return (1);
    }

	Config config;
	std::string configFile = argv[1];

	if (!config.parse(argv[1]))
	{
		logError("Failed to parse config file: " + configFile);
		return (1);
	}

    if (!config.validate())
    {
        logError("Config validation failed");
        return 1;
    }

	config.print();

	const std::vector<ServerConfig>& servers = config.getServers();
    try
    {
        Server server(servers);
        server.run();
    }
    catch (const std::exception& e)
    {
        logError(std::string("Fatal error: ") + e.what());
        return 1;
    }

    return 0;
}