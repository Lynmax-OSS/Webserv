#include "Webserv.hpp"

int main()
{
	int port = 8080;
	std::string root = "./www";
	std::string index = "index.html";

	Socket server(AF_INET, SOCK_STREAM, 0);

	if (server.fd == -1)
	{
		std::cout << "socket() failed: " << strerror(errno) << std::endl;
		return (1);
	}

	std::cout << "Socket created successfully. fd = " << server.fd << "\n";
    int opt = 1;
    if (setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "setsockopt() failed: " << strerror(errno) << "\n";
        close(server.fd);
        return 1;
    }

    std::cout << "SO_REUSEADDR enabled.\n";

	sockaddr_in addr{};

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s.addr =  INADDR_ANY;

	if (bind(server.fd, (sockaddr*)&addr, sizeof(addr)) == -1)
	{
		std::cout << "bind() failed: " << strerror(errno) << std::endl;
		close(server.fd);
		return (1);
	}
	return (0);
}