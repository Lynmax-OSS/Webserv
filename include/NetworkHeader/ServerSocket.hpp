#ifndef SERVERSOCKET_HPP
# define SERVERSOCKET_HPP
# include "../ConfigHeader/ConfigParser.hpp"


class ServerSocket
{
	public:
		int	fd;
		ServerSocket(const ServerConfig &config);
		~ServerSocket();

	private:
		ServerSocket();
		ServerSocket(const ServerSocket &other);
		ServerSocket &operator=(const ServerSocket &other);

};

#endif