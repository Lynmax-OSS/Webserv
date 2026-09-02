#include "../../include/Webserv.hpp"
#include "../../include/NetworkHeader/PollManager.hpp"

// ============================================================
// Constructor / Destructor
// ============================================================

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
// Poll management
// ============================================================

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
            return;
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

void PollManager::handleNewConnections(int server_fd)
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

    // Set non-blocking
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    std::cout << "Client connected on fd " << client_fd << "\n";
    _clients[client_fd] = ClientConnection(client_fd);
    addFd(client_fd, POLLIN | POLLOUT);
}

// ============================================================
// Client I/O
// ============================================================

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
        return;
    }

    ClientConnection& conn = _clients[fd];
    while (conn.getParser().isComplete())
    {
        processRequest(fd);

        // If keep-alive and response sent fully, continue to next request
        if (!conn.isKeepAlive() || !conn.getWriteBuffer().empty())
            break;
    }
}

void PollManager::handleClientWrite(int fd)
{
    sendResponse(fd);
}

// ============================================================
// Request handling
// ============================================================

void PollManager::processRequest(int fd)
{
    ClientConnection& conn = _clients[fd];
    const HttpRequest& req = conn.getParser().getRequest();

    if (req.error_code != 0)
    {
        // Build error response
        std::ostringstream error_stream;
        error_stream << "HTTP/1.1 " << req.error_code << " Error\r\n"
                    << "Content-Type: text/html\r\n"
                    << "Connection: close\r\n"
                    << "\r\n"
                    << "<h1>Error " << req.error_code << "</h1>";
        conn.getWriteBuffer() = error_stream.str();
        conn.setKeepAlive(false);   // close after error
    }
    else
    {
        std::cout << "✅ Received: " << req.method << " " << req.path << std::endl;
        handleRequest(req, _configs, conn.getWriteBuffer());
    }

    // Attempt to send the response immediately
    sendResponse(fd);

    // If keep-alive and all data sent, prepare parser for next request
    if (conn.isKeepAlive() && conn.getWriteBuffer().empty())
    {
        conn.getParser().reset(true);   // keep leftover buffer
        conn.getParser().feed("");      // process any pipelined data
    }
    else
    {
        // If not keep-alive, close connection
        closeConnection(fd);
    }
}

void PollManager::sendResponse(int fd)
{
    ClientConnection& conn = _clients[fd];
    if (conn.getWriteBuffer().empty())
        return;

    ssize_t written = write(fd, conn.getWriteBuffer().c_str(),
                            conn.getWriteBuffer().size());
    if (written > 0)
        conn.getWriteBuffer().erase(0, written);
    // If written == -1 && errno == EAGAIN, keep buffer for next POLLOUT
}

// ============================================================
// Cleanup
// ============================================================

void PollManager::closeConnection(int fd)
{
    std::cout << "Closing connection on fd " << fd << "\n";
    removeFd(fd);
    close(fd);
    _clients.erase(fd);
}

// ============================================================
// Main handler (to be implemented by Member 3)
// ============================================================

void PollManager::handleRequest(const HttpRequest& req, const std::vector<ServerConfig>& configs, std::string& response)
{
    // *************************************************************
    //  THIS IS THE METHOD MEMBER 3 MUST IMPLEMENT
    //  -----------------------------------------------------------
    //  They should:
    //    - Find the matching ServerConfig (by port or Host header)
    //    - Find the matching LocationConfig (by req.path)
    //    - Check allowed methods
    //    - Serve static files, handle CGI, uploads, etc.
    //    - Fill `response` with the full HTTP response (including headers)
    // *************************************************************
    // For now, return 501 Not Implemented as a placeholder.
	
    (void)configs;  // not used yet

    if (req.method == "GET") {
        std::string base_path = "./www";  // temporary hardcode
        std::string file_path = base_path + req.path;
        if (!file_path.empty() && file_path[file_path.size() - 1] == '/')
            file_path += "index.html";

        std::ifstream file(file_path.c_str(), std::ios::binary);
        if (file.is_open()) {
            std::string body((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
            file.close();

            std::string content_type = "text/html";
            if (file_path.find(".css") != std::string::npos)
                content_type = "text/css";
            else if (file_path.find(".js") != std::string::npos)
                content_type = "application/javascript";
            else if (file_path.find(".png") != std::string::npos)
                content_type = "image/png";
            // ... etc

            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: " << content_type << "\r\n"
                << "Content-Length: " << body.size() << "\r\n"
                << "Connection: keep-alive\r\n"
                << "\r\n"
                << body;
            response = oss.str();
        } else {
            response = "HTTP/1.1 404 Not Found\r\n"
                       "Content-Type: text/html\r\n"
                       "Content-Length: 0\r\n"
                       "Connection: close\r\n"
                       "\r\n";
        }
    } else {
        response = "HTTP/1.1 405 Method Not Allowed\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: 0\r\n"
                   "Connection: close\r\n"
                   "\r\n";
    }
}

// ============================================================
// Main loop
// ============================================================

void PollManager::run()
{
    std::cout << "Server running...\n";
    while (true)
    {
        int ready = poll(_poll_fds.data(), _poll_fds.size(), 1000);

        // Timeout checks
        time_t now = time(NULL);
        for (std::map<int, ClientConnection>::iterator it = _clients.begin(); it != _clients.end();)
        {
            if (it->second.isTimeout(now, 60))
            {
                std::cout << "Timeout on fd " << it->first << std::endl;
				int fd = it->first;
				++it;
                closeConnection(fd);
            }
            else
            {
                ++it;
            }
        }

        if (ready == -1)
            throw std::runtime_error("poll() failed");

        for (size_t i = 0; i < _poll_fds.size(); i++)
        {
            if (_poll_fds[i].revents == 0)
                continue;

            int fd = _poll_fds[i].fd;
            if (_poll_fds[i].revents & POLLIN)
            {
                if (isServerFd(fd))
                    handleNewConnections(fd);
                else
                    handleClientRead(fd);
            }
            if (_poll_fds[i].revents & POLLOUT)
            {
                if (!isServerFd(fd))
                    handleClientWrite(fd);
            }
        }
    }
}
