#include "../../include/ConfigHeader/ConfigParser.hpp"

std::vector<std::string>	tokenizer(const std::string &filepath)
{
	std::ifstream file(filepath.c_str());

	if (!file.is_open())
		throw ConfigException("Cannot open file");
	std::vector<std::string>	tokens;
	std::string					line;

	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		std::string token;
		while (iss >> token)
		{
			if (token[0] == '#')
				break;
			if (token == "{" || token == "}")
				tokens.push_back(token);
			else if (token[token.size() - 1] == '{')
			{
				tokens.push_back(token.substr(0, token.size() - 1));
				tokens.push_back("{");
			}
			else if (token[token.size() - 1] == ';')
				tokens.push_back(token.substr(0, token.size() - 1));
			else
				tokens.push_back(token);
		}
	}
	return (tokens);
}