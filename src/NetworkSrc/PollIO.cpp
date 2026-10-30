#include "../../include/Webserv.hpp"
#include "../../include/NetworkHeader/PollManager.hpp"
#include "../../include/ParserHeader/ClientConnection.hpp"

// Accept new connections
void PollManager::handleNewConnections(int server_fd)
{   //prepare a place to store client info
    sockaddr_in client_addr; //make empty box to hold client's add(IP + port) .. OS will fill it in
    socklen_t   client_len = sizeof(client_addr); //Tell the OS how big the box is
    std::memset(&client_addr, 0, sizeof(client_addr)); //fill the box with zeros - clean it first
    //accept the client - give them a new fd and fill client_addr with client's IP + Port
    // int flags = fcntl(client_fd, F_GETFL, 0); //get current flags
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1)
    {
        std::cerr << "accept() failed: " << strerror(errno) << "\n";
        return ;
    }
    // Set non-blocking //fcntl = file control - tool to look at or change flags
    // int flags = fcntl(client_fd, F_GETFL, 0); //get current flags
    fcntl(client_fd, F_SETFL, O_NONBLOCK); //set with new values - keeps existing flags, add nonblocker

    std::cout << "Client connected on fd " << client_fd << "\n";
    _clients[client_fd] = ClientConnection(client_fd);
    addFd(client_fd, POLLIN | POLLOUT); //watch this fd for read and write
}

// Reading from a client
ssize_t PollManager::readFromClient(int fd)
{
    char buf[4096]; //make temporary bucket that holds a set amount of bytes || cause one page size on linux is 4096 bytes
    std::memset(buf, 0, sizeof(buf)); //zeroed/clean

    ssize_t bytes = read(fd, buf, sizeof(buf) - 1); //read up to 4095 bytes
    if (bytes <= 0)//0 = disconnect, -1 = error,
        return bytes; //return to caller

    std::map<int, ClientConnection>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return -1;

    it->second.getParser().feed(std::string(buf, bytes));
    it->second.updateActivity();
    return bytes;
    // "GET / HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n"
    // Offset	Bytes	                                            ASCII
    // 0	    G E T / H T T P / 1 . 1 \r \n	                    GET / HTTP/1.1\r\n
    // 16	    H o s t : l o c a l h o s t : 8 0 8 0 \r \n	        Host: localhost:8080\r\n
    // 38	    U s e r - A g e n t : c u r l / 7 . 8 1 . 0 \r \n	User-Agent: curl/7.81.0\r\n
    // 62	    A c c e p t : * / * \r \n	                        Accept: */*\r\n
    // 75	    \r \n	                                            \r\n
    // 77->4095	\0 (still zeroed)
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
