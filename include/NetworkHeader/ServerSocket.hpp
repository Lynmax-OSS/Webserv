#ifndef SERVERSOCKET
# define SERVERSOCKET
# include "../ConfigHeader/ConfigParser.hpp"


class ServerSocket
{
	public:
		int	fd;
		ServerSocket(const ServerConfig &config);
		~ServerSocket();

	private:
		ServerSocket(const ServerSocket &other);
		ServerSocket &operator=(const ServerSocket &other);

};

#endif