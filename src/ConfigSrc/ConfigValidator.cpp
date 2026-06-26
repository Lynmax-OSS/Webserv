#include "../../include/ConfigHeader/ConfigParser.hpp"

void	validateServer(ServerConfig config)
{
	if (config.port < 1 || config.port > 65535)
		throw std::runtime_error("Invalid port number: " + config.port);
	else if (config.root.empty())
		throw std::runtime_error("No root directive to be found");
	else if (config.locations.empty())
		throw std::runtime_error("No location directive to be found");
}

void	ConfigValidator(std::vector<ServerConfig> &configs)
{
	if (configs.empty())
		throw	std::runtime_error("Server configs are empty");
	for (size_t i; i < configs.size() ; i++)
	{
		validateServer(configs[i]);
	}
}