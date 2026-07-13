#include "../../include/ConfigHeader/ConfigParser.hpp"
#include "../../include/Webserv.hpp"

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
			local.cgi_path = tokens[++i];
		i++;
	}
	i++;
	return (local);
}

size_t	parseSize(const std::string &val)
{
	char *end;
	unsigned long	num = std::strtoul(val.c_str(), &end, 10);

	if (end == val.c_str())
		throw std::runtime_error("Invalid size value: " + val);

	if (*end == 'M' || *end == 'm')
		return (num * 1024 * 1024);
	if (*end == 'K' || *end == 'k')
		return (num * 1024);
	if (*end == 'G' || *end == 'g')
		return (num * 1024 * 1024 * 1024);
	if (*end == '\0')
		return (num);
	throw std::runtime_error("Invalid size suffix: " + val);
}

ServerConfig parseServerBlock(const std::vector<std::string> &tokens, size_t &i)
{
	ServerConfig config;

	while (i < tokens.size() && tokens[i] != "}")
	{
		if (tokens[i] == "listen")
		{
			char *end;
			long val = std::strtol(tokens[++i].c_str(), &end, 10);
			if (*end != '\0')
				throw std::runtime_error("Invalid port: " + tokens[i]);
			config.port = static_cast<int>(val);
		}
		else if (tokens[i] == "server_name")
			config.server_name = tokens[++i];
		else if (tokens[i] == "root")
			config.root = tokens[++i];
		else if (tokens[i] == "index")
		{
			++i;
			while (i < tokens.size() && tokens[i] != "}"
				&& tokens[i] != "listen"
				&& tokens[i] != "server_name"
				&& tokens[i] != "root"
				&& tokens[i] != "error_page"
				&& tokens[i] != "client_max_body_size"
				&& tokens[i] != "location")
				config.index.push_back(tokens[i++]);
			continue;
		}
		else if (tokens[i] == "location")
		{
			config.locations.push_back(parseLocationBlock(tokens, i));
			continue;
		}
		else if (tokens[i] == "error_page")
		{
			char *end;
			long code = std::strtol(tokens[++i].c_str(), &end, 10);
			if (*end != '\0')
				throw std::runtime_error("Invalid error code");
			config.errors[static_cast<int>(code)] = tokens[++i];
		}
		else if (tokens[i] == "client_max_body_size")
			config.client_max_body_size = parseSize(tokens[++i]);
		i++;
	}
	i++;
	return (config);
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
	return (configs);
}

std::vector<ServerConfig>	ConfigParser(std::string configpath)
{
	std::vector<std::string> tokens = tokenizer(configpath);
	// for (int i = 0; !tokens[i].empty(); i++)
	// {
	// 	std::cout << tokens[i] << std::endl;
	// }
	std::vector<ServerConfig> configs = parse(tokens);
	ConfigValidator(configs);
	return (configs);
}