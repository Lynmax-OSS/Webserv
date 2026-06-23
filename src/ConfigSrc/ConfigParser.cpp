#include "../../include/ConfigHeader/ConfigParser.hpp"

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