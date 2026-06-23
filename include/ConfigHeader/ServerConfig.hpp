#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP
# include "LocationConfig.hpp"
# include <string>
# include <vector>
# include <map>

struct ServerConfig
{
	int port;
	std::string host;
	std::string server_name;
	std::string root;
	std::vector <std::string> index;
	std::vector <LocationConfig> locations;
	std::map <int, std::string> errors;
	size_t client_max_body_size;

	ServerConfig() : port(80), client_max_body_size(1 * 1024 * 1024) {}
};


#endif