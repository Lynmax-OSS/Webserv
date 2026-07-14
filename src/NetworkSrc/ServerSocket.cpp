#include "../../include/Webserv.hpp"
#include "../../include/NetworkHeader/ServerSocket.hpp"

ServerSocket::ServerSocket(const ServerConfig &config): fd(-1)
{
	Socket server(AF_INET, SOCK_STREAM, 0);

	if (server.fd == -1)
		throw std::runtime_error ("socket failed");
	int opt = 1;
	if (setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw (std::runtime_error("setsockopt failed"));
	sockaddr_in addr{};

	addr.sin_family = AF_INET;
	addr.sin_port = htons(config.port);
	addr.sin_addr.s_addr =  INADDR_ANY;

	if (bind(server.fd, (sockaddr*)&addr, sizeof(addr)) == -1)
		throw (std::runtime_error("bind failed"));
	if (listen(server.fd, 10) == -1)
		throw (std::runtime_error("listen failed"));
}

ServerSocket::~ServerSocket()
{
	if (fd != -1)
		close (fd);
}

