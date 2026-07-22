#include "../../include/Webserv.hpp"
#include "../../include/NetworkHeader/PollManager.hpp"

PollManager::PollManager(SocketManager &manager): _manager(manager)
{
	const std::vector<ServerSocket*> &socket = manager.getSocket();
	for (size_t i = 0; i < socket.size(); i++)
		addFd(socket[i]->fd, POLLIN);
}

PollManager::~PollManager()
{}

PollManager::PollManager(const PollManager &other): _manager(other._manager), _poll_fds(other._poll_fds)
{}

PollManager &PollManager::operator=(const PollManager &other)
{
	*this = other;
	return (*this);
}

void	PollManager::addFd(int fd, short events)
{
	pollfd pfd;

	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;
	_poll_fds.push_back(pfd);
}

void	PollManager::removeFd(int fd)
{
	for (size_t i = 0; i < _poll_fds.size(); i++)
	{
		if (_poll_fds[i].fd == fd)
		{
			_poll_fds.erase(_poll_fds.begin() + i);
			return ;
		}
	}
}

bool	PollManager::isServerFd(int fd) const
{
	const std::vector<ServerSocket*> &sockets = _manager.getSocket();
	for (size_t i = 0; i < sockets.size(); i++)
	{
		if (sockets[i]->fd == fd)
			return (true);
	}
	return (false);
}

void	PollManager::handleNewConnections(int server_fd)
{
	
}