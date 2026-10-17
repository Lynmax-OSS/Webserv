#include "../../include/Webserv.hpp"
#include "../../include/NetworkHeader/PollManager.hpp"


PollManager::PollManager(SocketManager &manager, const std::vector<ServerConfig> &configs)
    : _manager(manager), _configs(configs)
{
    const std::vector<ServerSocket*> &socket = manager.getSocket();
    for (size_t i = 0; i < socket.size(); i++)
        addFd(socket[i]->fd, POLLIN);
}

PollManager::~PollManager() {}

PollManager::PollManager(const PollManager &other)
    : _manager(other._manager), _poll_fds(other._poll_fds),
      _clients(other._clients), _configs(other._configs)
{}

PollManager &PollManager::operator=(const PollManager &other)
{
    if (this != &other)
    {
        _poll_fds = other._poll_fds;
        _clients = other._clients;
        _configs = other._configs;
    }
    return (*this);
}

// ============================================================
// Main loop
void PollManager::run() //infinite loop that asks the OS if anyone is ready, then handles them
{
    std::cout << "Server running...\n";
    while (true)
    {	int ready = poll(_poll_fds.data(), _poll_fds.size(), 1000);
		// poll() sleeps until something happens, or until 1 second passes.
		// and only wakes up when is triggered when any watched fd has news
		// and will timeout after 1 second if no event on any watched fd
        
		// Timeout checks
		kickIdleClients(time(NULL), 60);
        if (ready == -1)
            throw std::runtime_error("poll() failed");

        for (size_t i = 0; i < _poll_fds.size(); i++)
        {	if (_poll_fds[i].revents == 0)
                continue;
            int fd = _poll_fds[i].fd;
            if (_poll_fds[i].revents & POLLIN)
            {   if (isServerFd(fd))
                    handleNewConnections(fd);
                else
                {	handleClientRead(fd);
                    continue ; } }

            if (_poll_fds[i].revents & POLLOUT)
            {	if (!isServerFd(fd))
                    handleClientWrite(fd);
            }
        }
    }
}
