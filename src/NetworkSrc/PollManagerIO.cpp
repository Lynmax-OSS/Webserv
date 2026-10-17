#include "../../include/Webserv.hpp"
#include "../../include/NetworkHeader/PollManager.hpp"

// Accept new connections
void PollManager::handleNewConnections(int server_fd)
{
    sockaddr_in client_addr;
    socklen_t   client_len = sizeof(client_addr);
    std::memset(&client_addr, 0, sizeof(client_addr));

    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1)
    {
        std::cerr << "accept() failed: " << strerror(errno) << "\n";
        return ;
    }

    // Set non-blocking
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    const std::vector<ServerSocket*> &sockets = _manager.getSocket();
    size_t socket_index = 0;
    while (socket_index < sockets.size() && sockets[socket_index]->fd != server_fd)
        ++socket_index;
    if (socket_index == sockets.size() || socket_index >= _configs.size())
    {
        std::cerr << "No configuration found for server fd " << server_fd << "\n";
        close(client_fd);
        return ;
    }

    std::cout << "Client connected on fd " << client_fd << "\n";
    _clients[client_fd] = ClientConnection(
        client_fd, _configs[socket_index].client_max_body_size);
    addFd(client_fd, POLLIN | POLLOUT);
}

// Reading from a client
ssize_t PollManager::readFromClient(int fd)
{
    char buf[4096];
    std::memset(buf, 0, sizeof(buf));

    ssize_t bytes = read(fd, buf, sizeof(buf) - 1);
    if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return 0;                 // no data ready, treat as 0 bytes read (not an error)
    if (bytes <= 0)
        return bytes;             // 0 or negative -> disconnect or error

    ClientConnection& conn = _clients[fd];
    conn.getParser().feed(std::string(buf, bytes));
    conn.updateActivity();
    return bytes;
}

void PollManager::handleClientRead(int fd)
{
    ssize_t bytes = readFromClient(fd);
    if (bytes <= 0)
    {
        if (bytes < 0)            // actual read error
            std::cerr << "read error on fd " << fd << ": " << strerror(errno) << "\n";
        closeConnection(fd);
        return ;
    }

    ClientConnection& conn = _clients[fd];
    if (conn.getParser().getRequest().error_code == 413)
    {
        closeConnection(fd);
        return ;
    }
    while (conn.getParser().isComplete())
    {
        processRequest(fd);

        // If keep-alive and response sent fully, continue to next request
        if (!conn.isKeepAlive() || !conn.getWriteBuffer().empty())
            break;
    }
}

// Writing to a client
void PollManager::handleClientWrite(int fd)
{
    sendResponse(fd);
}

void PollManager::sendResponse(int fd)
{
    ClientConnection& conn = _clients[fd];
    if (conn.getWriteBuffer().empty())
        return ;

    ssize_t written = write(fd, conn.getWriteBuffer().c_str(),
                            conn.getWriteBuffer().size());
    if (written > 0)
        conn.getWriteBuffer().erase(0, written);
    // If written == -1 && errno == EAGAIN, keep buffer for next POLLOUT
}