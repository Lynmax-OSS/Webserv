#include "../../include/ConfigHeader/ConfigParser.hpp"

LocationConfig	parseLocationBlock(std::vector<std::string> tokens, size_t &i)
{
	LocationConfig local;

	local.path = tokens[++i];
	if (tokens[++i] != "{")
		throw std::runtime_error("Expected '{' after location path");
	++i;
	while (i < tokens.size() && tokens[i] != "}")
	{
		if (tokens[i] == "root")
			local.root = tokens[++i];
		else if (tokens[i] == "allowed_methods")
		{
			++i;
			while (i < tokens.size() && tokens[i] != "}" 
				   && tokens[i] != "autoindex"
				   && tokens[i] != "root"
				   && tokens[i] != "cgi_extension"
				   && tokens[i] != "cgi_path") {
				local.allowed_methods.push_back(tokens[i++]);
			}
			continue;
		}
		else if (tokens[i] == "autoindex")
		{
			i++;
			if (tokens[i] != "on" && tokens[i] != "off")
				throw std::runtime_error("Invalid autoindex value: " + tokens[i]);
			local.autoindex = (tokens[i] == "on");
		}
		else if (tokens[i] == "cgi_extension")
			local.cgi_extension = tokens[++i];
		else if (tokens[i] == "cgi_path")
			local.cgi_path == tokens[++i];
	}
}

ServerConfig	parseServerBlock(const std::vector<std::string> &tokens, size_t &i)
{
	ServerConfig config;

	while (i < tokens.size() && tokens[i] != "}")
	{
		if (tokens[i] == "listen")
			config.port = std::atoi(tokens[++i].c_str());
		else if (tokens[i] == "server_name")
			config.server_name = tokens[++i];
		else if (tokens[i] == "root")
			config.root = tokens[++i];
		else if (tokens[i] == "location")
			config.locations.push_back(parseLocationBlock(tokens, i));
		i++;
	}
}

std::vector<ServerConfig>	parse(const std::vector<std::string>& tokens)
{
	size_t i = 0;
	std::vector<ServerConfig> configs;

	while (i < tokens.size())
	{
		if (tokens[i] == "server" && tokens[i + 1] == "{")
		{
			i += 2;
			configs.push_back(parseServerBlock(tokens, i));
		}
		else
			throw std::runtime_error("Unexpected token: " + tokens[i]);
	}
	
}

std::vector<ServerConfig>	ConfigParser(std::string configpath)
{
	std::vector<std::string> tokens = tokenizer(configpath);


}