#include "../include/Webserv.hpp"

int main()
{
	try
	{
		std::vector<ServerConfig> conf = ConfigParser("configs/default.conf");

		Socket server(AF_INET, SOCK_STREAM, 0);

		if (server.fd == -1)
		{
			std::cout << "socket() failed: " << strerror(errno) << std::endl;
			return (1);
		}
		std::cout << "Socket created successfully. fd = " << server.fd << "\n";
		int opt = 1;
		if (setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		{
			std::cerr << "setsockopt() failed: " << strerror(errno) << "\n";
			close(server.fd);
			return 1;
		}
		std::cout << "SO_REUSEADDR enabled.\n";
		sockaddr_in addr{};

		addr.sin_family = AF_INET;
		addr.sin_port = htons(conf[0].port);
		addr.sin_addr.s_addr =  INADDR_ANY;

		if (bind(server.fd, (sockaddr*)&addr, sizeof(addr)) == -1)
		{
			std::cout << "bind() failed: " << strerror(errno) << std::endl;
			close(server.fd);
			return (1);
		}
		if (listen(server.fd, 10) == -1)
		{
			std::cerr << "listen() failed: " << strerror(errno) << "\n";
			close(server.fd);
			return (1);
		}	
		std::cout << "Listening on port " << conf[0].port << "...\n";
		while (true)
		{
			sockaddr_in	client_addr{};
			socklen_t	client_len = sizeof(client_addr);

			int client_fd = accept(server.fd, (sockaddr*)&client_addr, &client_len);
			if (client_fd == -1)
			{
				std::cerr << "accept() failed: " << strerror(errno) << "\n";
				continue;
			}
			std::cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << " on fd " << client_fd << "\n";
			char buf[4096] = {};
			ssize_t bytes = read(client_fd, buf, sizeof(buf) - 1);
			if (bytes > 0)
				std::cout << "Received:\n" << buf << "\n";
			std::string response =
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/html\r\n"
				"Connection: close\r\n"
				"\r\n"
				"<h1>Hello from your webserver!</h1>";
			write(client_fd, response.c_str(), response.size());
			close(client_fd);
		}
		close(server.fd);
	}
	catch(const ConfigException& e)
	{
		std::cout << "Config error: " << e.what() << std::endl;
	}
	catch(const std::exception& e)
	{
		std::cout << "Unkown error: " << e.what() << std::endl;
	}
	return (0);
}