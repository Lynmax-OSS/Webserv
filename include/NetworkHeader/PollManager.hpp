#ifndef POLLMANAGER_HPP
# define POLLMANAGER_HPP

# include "../NetworkHeader/ServerSocket.hpp"
# include "../NetworkHeader/SocketManager.hpp"
# include "../ParserHeader/ClientConnection.hpp"
# include <poll.h>
# include <map>
# include <sstream>
# include <fstream>
# include <fcntl.h>
# include <vector>

class PollManager
{
	public:
		PollManager(SocketManager &manager, const std::vector<ServerConfig> &configs);
		~PollManager();
		PollManager(const PollManager &other);
		PollManager &operator=(const PollManager &other);
		void	run();

	private:
		SocketManager	&_manager;
		std::vector<pollfd> _poll_fds;
		std::map<int, ClientConnection> _clients;
		std::vector<ServerConfig> _configs;

		// Poll management
		void	addFd(int fd, short events);
		void	removeFd(int fd);
		bool	isServerFd(int fd) const;
		void	handleNewConnections(int server_fd);

		// Client I/O
		ssize_t	readFromClient(int fd);
		void	handleClientRead(int fd);
		void	handleClientWrite(int fd);

		// Request handling
		void	processRequest(int fd);   // NEW: processes one complete request
		void	sendResponse(int fd);     // NEW: writes buffer to socket

		// Cleanup
		void	closeConnection(int fd);

		// Main handler to be implemented by Member 3
		void	handleRequest(const HttpRequest& req, const std::vector<ServerConfig>& configs, std::string& response);
};

#endif
