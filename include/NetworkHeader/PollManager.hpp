#ifndef POLLMANAGER_HPP
# define POLLMANAGER_HPP
# include "../NetworkHeader/ServerSocket.hpp"
# include "../NetworkHeader/SocketManager.hpp"
# include <poll.h>

class PollManager
{
	public:
		PollManager();
		~PollManager();
		PollManager(const PollManager &other);
		PollManager &operator=(const PollManager &other);
	private:
		SocketManager	&_manager;
		std::vector<pollfd> _poll_fds;

};

#endif