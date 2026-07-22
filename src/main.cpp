#include "../include/Webserv.hpp"
#include "../include/NetworkHeader/PollManager.hpp"
#include "../include/NetworkHeader/SocketManager.hpp"

int main(int argc, char **argv)
{
	try
	{
		std::string path = argc > 1 ? argv[1] : "configs/default.conf";
		
		std::vector<ServerConfig> configs = ConfigParser(path);
		SocketManager manager(configs);
		PollManager poll(manager);
		poll.run();
	}
	catch(const ConfigException& e)
		{ std::cerr << "Config error: " << e.what() << std::endl; }
	catch(const std::exception& e)
		{ std::cerr << "Unknown error: " << e.what() << std::endl; }
	return (0);
}