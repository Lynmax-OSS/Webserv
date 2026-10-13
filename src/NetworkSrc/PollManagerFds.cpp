#include "../../include/Webserv.hpp"
#include "../../include/NetworkHeader/PollManager.hpp"
#include <unistd.h>

// Poll fd bookkeeping
void PollManager::addFd(int fd, short events)
{
    pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;
    pfd.revents = 0;
    _poll_fds.push_back(pfd);
}

void PollManager::removeFd(int fd)
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

bool PollManager::isServerFd(int fd) const
{
    const std::vector<ServerSocket*> &sockets = _manager.getSocket();
    for (size_t i = 0; i < sockets.size(); i++)
    {
        if (sockets[i]->fd == fd)
            return (true);
    }
    return (false);
}

// Cleanup
void PollManager::closeConnection(int fd)
{
    std::cout << "Closing connection on fd " << fd << "\n";
    removeFd(fd);
    close(fd);
    _clients.erase(fd);
}
