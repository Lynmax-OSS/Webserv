#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP
# include <vector>
# include <iostream>

struct LocationConfig
{
	std::string	path;
	std::string	root;
	std::vector <std::string> allowed_methods;
	bool autoindex;
	std::string cgi_extension;
	std::string cgi_path;

	LocationConfig() : autoindex(false) {}
};


#endif