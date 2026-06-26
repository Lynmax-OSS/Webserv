#include "../../include/ConfigHeader/ConfigParser.hpp"

void	validateLocation(LocationConfig local)
{
	if (local.path.empty() || local.path[0] != '/')
		throw std::runtime_error("No path to directory");
	if (local.cgi_extension.empty() || local.cgi_path.empty())
		throw std::runtime_error("Both cgi path and extenstion must be config");
	for (int i = 0; i < local.allowed_methods.size(); i++)
	{
		const std::string &m = local.allowed_methods[i];
		if (m != "GET" && m != "POST" && m != "DELETE")
			throw std::runtime_error("Invalid method: " + m); 
	}
}

void	validateServer(ServerConfig config)
{
	if (config.port < 1 || config.port > 65535)
		throw std::runtime_error("Invalid port number: " + config.port);
	else if (config.root.empty())
		throw std::runtime_error("No root directive to be found");
	else if (config.locations.empty())
		throw std::runtime_error("No location directive to be found");
	for (int i = 0; i < config.locations.size(); i++)
		validateLocation(config.locations[i]);
}

void	checkDupPort(std::vector<ServerConfig> config)
{
	for (size_t i = 0; i < config.size(); i++)
	{
		for (size_t j = i + 1; j < config.size(); j++)
		{
			if (config[i].port == config[j].port)
				throw std::runtime_error("Duplicate port detected");
		}
	}
	
}

void	ConfigValidator(std::vector<ServerConfig> &configs)
{
	if (configs.empty())
		throw	std::runtime_error("Server configs are empty");
	for (size_t i; i < configs.size() ; i++)
		validateServer(configs[i]);
	checkDupPort(configs);
}