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
		PollManager poll(manager, configs);
		poll.run();
	}
	catch(const ConfigException& e)
		{ std::cerr << "Config error: " << e.what() << std::endl; }
	catch(const std::exception& e)
		{ std::cerr << "Unknown error: " << e.what() << std::endl; }
	return (0);
}

// #include "../include/ParserHeader/HttpParser.hpp"
// #include <iostream>
// #include <unistd.h>

// int main() {
//     alarm(2);   // if we haven't finished in 2s, kill ourselves — proves the hang

//     HttpParser p;

//     // Deliberately incomplete: no "\r\n" yet, so _processRequestLine
//     // can't consume anything. If feed() lacks the progress guard,
//     // this call spins forever.
//     std::cout << "Feeding partial request line...\n" << std::flush;
//     p.feed("GET / HTTP/1.1");
//     std::cout << "Returned from first feed (good — no hang)\n" << std::flush;

//     // Now finish the request line + headers, and confirm normal flow works
//     p.feed("\r\nHost: localhost\r\n\r\n");
//     std::cout << "complete=" << p.isComplete()
//               << " error="  << p.hasError()
//               << " code="   << p.getRequest().error_code
//               << " method=" << p.getRequest().method
//               << " path="   << p.getRequest().path
//               << "\n";
//     return 0;
// }
