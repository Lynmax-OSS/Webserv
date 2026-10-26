#include "../../include/Webserv.hpp"
#include "../../include/NetworkHeader/PollManager.hpp"

// Accept new connections
void PollManager::handleNewConnections(int server_fd)
{   //prepare a place to store client info
    sockaddr_in client_addr; //make empty box to hold client's add(IP + port) .. OS will fill it in
    socklen_t   client_len = sizeof(client_addr); //Tell the OS how big the box is
    std::memset(&client_addr, 0, sizeof(client_addr)); //fill the box with zeros - clean it first
    //accept the client - give them a new fd and fill client_addr with client's IP + Port
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1)
    {
        std::cerr << "accept() failed: " << strerror(errno) << "\n";
        return ;
    }
    // Set non-blocking //fcntl = file control - tool to look at or change flags
    int flags = fcntl(client_fd, F_GETFL, 0); //get current flags
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK); //set with new values - keeps existing flags, add nonblocker

    std::cout << "Client connected on fd " << client_fd << "\n";
    _clients[client_fd] = ClientConnection(client_fd);
    addFd(client_fd, POLLIN | POLLOUT); //watch this fd for read and write
}

// Reading from a client
ssize_t PollManager::readFromClient(int fd)
{
    char buf[4096]; //make temporary bucket that holds a set amount of bytes || cause one page size on linux is 4096 bytes
    std::memset(buf, 0, sizeof(buf)); //fill bucket with zeroes to clean it

    ssize_t bytes = read(fd, buf, sizeof(buf) - 1); //req OS to store up to 4095 bytes from this socket from buf
    // if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) //try again later || nothing to read, but keep the connection open
    //     return 0; // no data ready.. not an error
    if (bytes <= 0)//0 = disconnect, -1 = error,
        return bytes; //return to caller

    std::map<int, ClientConnection>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return -1;

    it->second.getParser().feed(std::string(buf, bytes));
    it->second.updateActivity();
    return bytes;
}

void PollManager::handleClientRead(int fd)
{
    ssize_t bytes = readFromClient(fd); //read the bytes
    if (bytes == 0)
    {
        std::map<int, ClientConnection>::iterator it = _clients.find(fd);
        if (it != _clients.end())
            it->second.setReadClosed(true);
        return ;
    }
    if (bytes < 0)
    {
        std::cerr << "read error on fd " << fd << "\n";
        closeConnection(fd);
        return ;
    }
    std::map<int, ClientConnection>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return ;
    ClientConnection& conn = it->second;
    while (conn.getParser().isComplete() || conn.getParser().hasError()) //repeat as long as a complete request is ready
    {
        if (!processRequest(fd)) //send the parsed request to be handled and replied to
            return ;
        // stop looping if connection isnt keep-alive or there's still response data not sent yet
        if (!conn.isKeepAlive() || !conn.getWriteBuffer().empty())
            break;
    }
}

void PollManager::sendResponse(int fd)
{
    std::map<int, ClientConnection>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return ;
    
    ClientConnection& conn = it->second;
    if (conn.getWriteBuffer().empty())
        return ;

    ssize_t written = write(fd, conn.getWriteBuffer().c_str(),
                            conn.getWriteBuffer().size());
    if (written < 0)
    {
        closeConnection(fd);
        return ;
    }
    if (written > 0)
        conn.getWriteBuffer().erase(0, written);
}
