#ifndef POLLMANAGER_HPP
# define POLLMANAGER_HPP

# include "../NetworkHeader/ServerSocket.hpp"
# include "../NetworkHeader/SocketManager.hpp"
# include "../ParserHeader/ClientConnection.hpp"
# include "../NetworkUtilsHeader/CGI.hpp"
# include <poll.h>
# include <map>
# include <sstream>
# include <fstream>
# include <fcntl.h>
# include <vector>
# include <sys/stat.h>
# include <ctime>

class PollManager
{
	public:
		PollManager(SocketManager &manager, const std::vector<ServerConfig> &configs);
		~PollManager();
		PollManager(const PollManager &other);
		PollManager &operator=(const PollManager &other);
		void	run();
		void	updatePollFds(void);

	private:
		SocketManager	&_manager;
		std::vector<pollfd> _poll_fds;
		std::map<int, ClientConnection> _clients;
		std::map<int, CGI> _cgi_jobs;
		std::map<int, int> _cgi_pipes;
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
		bool	processRequest(int fd);
		void	sendResponse(int fd);

		// Cleanup
		void	closeConnection(int fd);
		void	kickIdleClients(time_t now, int timeout_sec);

		// Main handler
		void	handleRequest(const HttpRequest& req, const std::vector<ServerConfig>& configs, std::string& response, ClientConnection& conn);
		std::string handleCGI(const HttpRequest &req, const ServerConfig *server, const LocationConfig *location, const std::string &scriptPath, ClientConnection &conn);
		bool	runCGI(std::string cgi_path, std::string script_path, const HttpRequest &req, int client_fd, const ServerConfig *server);
		void	handleCGIEvent(int fd, short revents);
		void	finishCGI(int clientFD);
		void	destroyCGIJob(int clientFD);
		void	checkCGITimeouts(time_t currentTime);
};

#endif
