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
	sockaddr_in client_addr;
	socklen_t   client_len = sizeof(client_addr);
	std::memset(&client_addr, 0, sizeof(client_addr));

	int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
	if (client_fd == -1)
	{
		std::cerr << "accept() failed: " << strerror(errno) << "\n";
		return;
	}
	std::cout << "Client connected on fd " << client_fd << "\n";
	addFd(client_fd, POLLIN | POLLOUT);
}

void	PollManager::handleClientRead(int client_fd)
{
	char buf[4096];
	std::memset(buf, 0, sizeof(buf));

	ssize_t bytes = read(client_fd, buf, sizeof(buf) - 1);
	if (bytes <= 0)
	{
		std::cout << "Client fd " << client_fd << " disconnected\n";
		close(client_fd);
		removeFd(client_fd);
		return;
	}
	std::cout << "Received:\n" << buf << "\n";
	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Connection: close\r\n"
		"\r\n"
		"<h1>Hello from webserv!</h1>";
	write(client_fd, response.c_str(), response.size());
	close(client_fd);
	removeFd(client_fd);
}

void PollManager::run()
{
	std::cout << "Server running...\n";
	while (true)
	{
		int ready = poll(_poll_fds.data(), _poll_fds.size(), -1);
		if (ready == -1)
			throw std::runtime_error("poll() failed");

		for (size_t i = 0; i < _poll_fds.size(); i++)
		{
			if (_poll_fds[i].revents == 0)
				continue;

			if (_poll_fds[i].revents & POLLIN)
			{
				if (isServerFd(_poll_fds[i].fd))
					handleNewConnections(_poll_fds[i].fd);
				else
					handleClientRead(_poll_fds[i].fd);
			}
		}
	}
}