#ifndef CONFIGPARSER_HPP
# define CONFIG_PARSER_HPP

#include "ServerConfig.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

std::vector<std::string>	tokenizer(const std::string &filepath);

class ConfigException : public std::runtime_error
{
	public:
		ConfigException(const std::string& msg): runtime_error(msg)
		{}
};

class WebservException : public std::runtime_error
{
	public:
		WebservException(const std::string& msg): std::runtime_error(msg)
		{}
};
#endif