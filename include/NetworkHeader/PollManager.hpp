#ifndef POLLMANAGER_HPP
# define POLLMANAGER_HPP
# include "../NetworkHeader/ServerSocket.hpp"
# include "../NetworkHeader/SocketManager.hpp"
# include <poll.h>

class PollManager
{
	public:
		PollManager();
		PollManager(SocketManager &manager);
		~PollManager();
		PollManager(const PollManager &other);
		PollManager &operator=(const PollManager &other);
		void	run();

	private:
		SocketManager	&_manager;
		std::vector<pollfd> _poll_fds;
		void	addFd(int fd, short events);
		void	removeFd(int fd);
		bool	isServerFd(int fd) const;
		void	handleNewConnections(int server_fd);
		void	handleClientRead(int client_fd);
};

#endif