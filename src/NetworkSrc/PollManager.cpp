#include "../../include/Webserv.hpp"
#include "../../include/NetworkHeader/PollManager.hpp"
#include "../../include/NetworkUtilsHeader/ResponseUtils.hpp"
#include "../../include/NetworkUtilsHeader/CGI.hpp"
# include <sys/wait.h>
# include <dirent.h>
# include <algorithm>

PollManager::PollManager(SocketManager &manager, const std::vector<ServerConfig> &configs)
    : _manager(manager), _configs(configs)
{   // Walk the vector, not fd numbers.
    //   i     		   → position in the vector
    //   socket[i]     → pointer to one ServerSocket
    //   socket[i]->fd → that socket's OS-assigned fd (e.g. 3, 7, ...)
    const std::vector<ServerSocket*> &socket = manager.getSocket(); //get list of socket
    for (size_t i = 0; i < socket.size(); i++)
        addFd(socket[i]->fd, POLLIN); //Watch that socket's fd for readable events
} //POLLIN - "readable" event flag || wake me on new connections


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

void PollManager::handleClientWrite(int fd)
{
    sendResponse(fd); // send bytes to kernel

    std::map<int, ClientConnection>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return ;

    ClientConnection& conn = it->second;
    if (conn.getWaitingForCGI())
        return ;
    if (!conn.getWriteBuffer().empty())
        return ;
    if (conn.isReadClosed())
    {
        closeConnection(fd);
        return ;
    }
    if (!conn.isKeepAlive())
    {
        closeConnection(fd);
        return ;
    }

    conn.getParser().reset(true); // parser is ready for next request
    conn.getParser().feed(""); //wake up parser to process any pipelined bytes already in _buffer
    applyBodyLimit(conn);

    if (conn.getParser().isComplete() || conn.getParser().hasError())
        processRequest(fd);
}

// Request handling

bool PollManager::processRequest(int fd)
{
    ClientConnection& conn = _clients[fd];
    const HttpRequest& req = conn.getParser().getRequest();
    std::cout << conn.getParser()._max_body_size << std::endl;

    if (req.error_code != 0)
    {
        conn.getWriteBuffer() = buildErrorResponse(req.error_code, selectServer(req), false);
        conn.setKeepAlive(false);   // close after error
    }
    else
    {
        std::cout << "✅ Received: " << req.method << " " << req.path << std::endl;
        handleRequest(req, conn.getWriteBuffer(), conn);
    }
    if (conn.getWaitingForCGI())
        return (true);
    else
	    return conn.isKeepAlive();
}

const ServerConfig *PollManager::selectServer(const HttpRequest &req) const
{
    std::string host;
    std::map<std::string, std::string>::const_iterator hostIt = req.headers.find("host");
    if (hostIt != req.headers.end())
        host = hostIt->second;
    int port = _configs.empty() ? 8080 : _configs[0].port;
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos)
        port = std::atoi(host.substr(colonPos + 1).c_str());
    std::string hostname = host.substr(0, colonPos);
    const ServerConfig *portMatch = NULL;
    for (size_t i = 0; i < _configs.size(); ++i)
    {
        if (_configs[i].port != port)
            continue;
        if (portMatch == NULL)
            portMatch = &_configs[i];
        if (!hostname.empty() && _configs[i].server_name == hostname)
            return &_configs[i];
    }
    if (portMatch != NULL)
        return portMatch;
    return _configs.empty() ? NULL : &_configs[0];
}

void PollManager::applyBodyLimit(ClientConnection &conn)
{
    HttpParser &parser = conn.getParser();
    if (!parser.needsBodyLimit())
        return;
    const ServerConfig *server = selectServer(parser.getRequest());
    parser.setMaxBodySize(server == NULL ? 1024 * 1024 : server->client_max_body_size);
    parser.feed("");
}

void PollManager::handleRequest(const HttpRequest& req, std::string& response, ClientConnection &conn)
{
    struct stat path_stat;
    std::string filePath;

    const ServerConfig* server = selectServer(req);
    if (server == NULL)
    {
        response = buildErrorResponse(500, NULL, false);
        return;
    }
    if (hasParentTraversal(req.path))
    {
        response = buildErrorResponse(403, server, req.keep_alive);
        return ;
    }
    const LocationConfig* location = NULL;
    {
        size_t longestMatch = 0;
        for (size_t i = 0; i < server->locations.size(); ++i)
        {
            const LocationConfig& loc = server->locations[i];
            if (req.path.compare(0, loc.path.size(), loc.path) == 0
                && loc.path.size() > longestMatch)
            {
                location = &loc;
                longestMatch = loc.path.size();
            }
        }
    }
    if (location != NULL && !location->allowed_methods.empty())
    {
        bool allowed = false;
        for (size_t i = 0; i < location->allowed_methods.size(); ++i)
        {
            if (req.method == location->allowed_methods[i])
            {
                allowed = true;
                break;
            }
        }
        if (!allowed)
        {
            response = buildErrorResponse(405, server, false);
            return;
        }
    }
    if (location != NULL && location->return_code != 0)
    {
        std::map<std::string, std::string> headers;
        headers["Location"] = location->return_url;
        response = buildResponse(location->return_code, headers, "", req.keep_alive);
        return;
    }

    if (location && !location->cgi_extension.empty() && location->cgi_extension == getExtension(req.uri))
    {
        response = handleCGI(req, server, location, resolvePath(server, location, req), conn);
        return ;
    }
    if (req.method == "GET")
    {
        std::string path = resolvePath(server, location, req);

        if (stat(path.c_str(), &path_stat) == -1)
        {
            response = buildErrorResponse(404, server, req.keep_alive);
            return ;
        }
        if (S_ISREG(path_stat.st_mode))
            filePath = path;
        else if (S_ISDIR(path_stat.st_mode))
        {
            std::string directory = path;
            if (path[path.length() - 1] == '/')
                path = path + server->index[0];
            else
                path = path + "/" + server->index[0];
            if (stat(path.c_str(), &path_stat) == 0 && S_ISREG(path_stat.st_mode))
                filePath = path;
            else
            {
                std::vector<std::string> autoIndexList;
                if (!location || !location->autoindex)
                {
                    response = buildErrorResponse(404, server, req.keep_alive);
                    return ;
                }
                DIR *dir = opendir(directory.c_str());
                if (dir == NULL)
                {
                    response = buildErrorResponse(403, server, req.keep_alive);
                    return ;
                }
                struct dirent *entry;

                while ((entry = readdir(dir)) != NULL)
                {
                    if (std::string(entry->d_name) == ".")
                        continue;
                    autoIndexList.push_back(entry->d_name);
                }
                std::sort(autoIndexList.begin(), autoIndexList.end());
                closedir(dir);
                std::string body = buildAutoIndexPage(req.path, directory, autoIndexList);
                std::map<std::string, std::string> headers;
                headers["Content-Type"] = "text/html";
                response = buildResponse(200, headers, body, req.keep_alive);
                return;
            }
        }
        else
        {
            response = buildErrorResponse(403, server, req.keep_alive);
            return ;
        }
        std::string body;
        if (!readFileToString(path, body))
        {
            response = buildErrorResponse(500, server, req.keep_alive);
            return ;
        }
        std::map<std::string, std::string> headers;
        headers["Content-Type"] = getMimeType(filePath);
        response = buildResponse(200, headers, body, req.keep_alive);
        return;
        // for (std::vector<std::string>::const_iterator it = server->index.begin() ; it != server->index.end() ; ++it)
        // {
        //     path = path + *it;
        //     std::cout << path << std::endl;
        // }
    }
    else if (req.method == "POST")
    {
        std::string fullPath = resolvePath(server, location, req);

        if (fullPath[fullPath.length() - 1] == '/')
        {
            response = buildErrorResponse(400, server, req.keep_alive);
            return ;
        }
        if (hasParentTraversal(req.path))
        {
            response = buildErrorResponse(403, server, req.keep_alive);
            return ;
        }
        if (req.content_length >= 0 && static_cast<size_t>(req.content_length) > server->client_max_body_size)
        {
            response = buildErrorResponse(413, server, req.keep_alive);
            return ;
        }
        size_t  slash = fullPath.rfind('/');
        std::string parentDir = fullPath.substr(0, slash);
        struct stat dirStat;
        if (stat(parentDir.c_str(), &dirStat) == -1)
        {
            response = buildErrorResponse(404, server, req.keep_alive);
            return ;
        }
        if (!S_ISDIR(dirStat.st_mode))
        {
            response = buildErrorResponse(404, server, req.keep_alive);
            return ;
        }
        if (access(parentDir.c_str(), W_OK) == -1)
        {
            response = buildErrorResponse(403, server, req.keep_alive);
            return ;
        }
        struct stat fileStat;
        if (stat(fullPath.c_str(), &fileStat) == 0 && S_ISDIR(fileStat.st_mode))
        {
            response = buildErrorResponse(400, server, req.keep_alive);
            return ;
        }
        fullPath = getUniqueFileName(fullPath);
        if (fullPath.empty())
        {
            response = buildErrorResponse(500, server, req.keep_alive);
            return;
        }
        std::ofstream   outFile(fullPath.c_str(), std::ios::binary);
        if (!outFile.is_open())
        {
            response = buildErrorResponse(500, server, req.keep_alive);
            return;
        }
        outFile.write(req.body.c_str(), req.body.size());
        outFile.close();
        if (outFile.fail())
        {
            response = buildErrorResponse(500, server, req.keep_alive);
            return ;
        }
        std::string savedName = fullPath.substr(fullPath.rfind('/') + 1);
        std::map<std::string, std::string> headers;
        headers["Location"] = req.path.substr(0, req.path.rfind('/') + 1) + savedName;
        response = buildResponse(201, headers, "", req.keep_alive);
        return;
    }
    else if (req.method == "DELETE")
    {
        std::string path = resolvePath(server, location, req);

        if (hasParentTraversal(req.path))
        {
            response = buildErrorResponse(403, server, req.keep_alive);
            return ;
        }
        if (stat(path.c_str(), &path_stat) == -1)
        {
            response = buildErrorResponse(404, server, req.keep_alive);
            return ;
        }
        if (!S_ISREG(path_stat.st_mode))
        {
            response = buildErrorResponse(403, server, req.keep_alive);
            return ;
        }
        size_t  slashIndex = path.find_last_of('/');
        std::string dir = (slashIndex == std::string::npos) ? "." : path.substr(0, slashIndex);
        if (access(dir.c_str(), W_OK) == -1)
        {
            response = buildErrorResponse(403, server, req.keep_alive);
            return ;
        }
        if (std::remove(path.c_str()) != 0)
        {
            response = buildErrorResponse(500, server, req.keep_alive);
            return ;
        }
        std::map<std::string, std::string> headers;
        response = buildResponse(204, headers, "", req.keep_alive);
        return;
    }
}

void PollManager::updatePollFds(void)
{
    for (std::vector<pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end();)
    {
        if (it->fd == -1)
            it = _poll_fds.erase(it);
        else
            ++it;
    }
    for (std::vector<pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); ++it)
    {
        if (isServerFd(it->fd))
            continue;
        std::map<int, ClientConnection>::iterator clientIt = _clients.find(it->fd);
        if (clientIt == _clients.end())
            continue;
        if (!clientIt->second.isReadClosed())
            it->events = POLLIN;
        if (clientIt->second.getWriteBuffer().size() > 0)
            it->events |= POLLOUT;
    }
}

void PollManager::handleCGIEvent(int fd, short revent)
{
    std::map<int, int>::iterator client = _cgi_pipes.find(fd);
    if (client == _cgi_pipes.end())
        return;
    int clientFD = client->second;
    std::map<int, CGI>::iterator job = _cgi_jobs.find(clientFD);
    if (job == _cgi_jobs.end())
    {
        close(fd);
        _cgi_pipes.erase(fd);
        for (std::vector<struct pollfd>::iterator pfd = _poll_fds.begin(); pfd != _poll_fds.end(); ++pfd)
        {
            if (pfd->fd == fd)
                pfd->fd = -1;
        }
    }
    else if (job->second.get_pipe_stdout() == fd)
    {
        if ((revent & POLLERR) || (revent & POLLNVAL))
        {
            close(fd);
            _cgi_pipes.erase(fd);
            job->second.set_pipe_stdout(-1);
            for (std::vector<struct pollfd>::iterator pfd = _poll_fds.begin(); pfd != _poll_fds.end(); ++pfd)
            {
                if (pfd->fd == fd)
                    pfd->fd = -1;
            }
            finishCGI(clientFD);
            return ;
        }
        if ((revent & POLLIN) || (revent & POLLHUP))
        {
            char	buf[4096];
            ssize_t readBytes = read(fd, buf, sizeof(buf));
            if (readBytes > 0)
                job->second.append_output(buf, readBytes);
            else
            {
                // std::cerr << "CGI Done, collected " << job->second.get_output().size() << " for client" << clientFD << std::endl;
                close(fd);
                _cgi_pipes.erase(fd);
                job->second.set_pipe_stdout(-1);
                for (std::vector<struct pollfd>::iterator pfd = _poll_fds.begin(); pfd != _poll_fds.end(); ++pfd)
                {
                    if (pfd->fd == fd)
                        pfd->fd = -1;
                }
                finishCGI(clientFD);
            }
        }
    }
    else if (job->second.get_pipe_stdin() == fd)
    {
        if ((revent & POLLERR) || (revent & POLLHUP) || (revent & POLLNVAL))
        {
            close(fd);
            _cgi_pipes.erase(fd);
            job->second.set_pipe_stdin(-1);
            for (std::vector<struct pollfd>::iterator pfd = _poll_fds.begin(); pfd != _poll_fds.end(); ++pfd)
            {
                if (pfd->fd == fd)
                    pfd->fd = -1;
            }
        }
        else if ((revent & POLLOUT))
        {
            const std::string &body = job->second.get_body();
            size_t      body_written = job->second.get_body_written();
            ssize_t     written = write(fd, body.c_str() + body_written, body.size() - body_written);
            if (written > 0)
                job->second.set_body_written(body_written + static_cast<size_t>(written));
            if (written <= 0 || job->second.get_body_written() == job->second.get_body().size())
            {
                close(fd);
                _cgi_pipes.erase(fd);
                job->second.set_pipe_stdin(-1);
                for (std::vector<struct pollfd>::iterator pfd = _poll_fds.begin(); pfd != _poll_fds.end(); ++pfd)
                {
                    if (pfd->fd == fd)
                        pfd->fd = -1;
                }
            }
        }
    }
}

void    PollManager::finishCGI(int clientFD)
{
    int status;
    int http_return_code;
    bool ok = false;
    std::string content_type;
    std::string body;
    std::string response;
    std::map<int, CGI>::iterator job = _cgi_jobs.find(clientFD);
    if (job == _cgi_jobs.end())
        return ;
    pid_t pid = job->second.get_child_pid();
    pid_t child_status = waitpid(pid, &status, WNOHANG);
    if (child_status == 0)
    {
        kill(pid, SIGKILL);
        child_status = waitpid(pid, &status, 0);
    }
    if (child_status > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0)
        ok = true;
    if (!ok)
        response = buildErrorResponse(500, job->second.get_server(), job->second.get_keep_alive());
    else
    {
        parseCgiOutput(job->second.get_output(), http_return_code, content_type, body);
        std::map<std::string, std::string> headers;
        headers["Content-Type"] = content_type;
        if (job->second.get_output().empty())
            response = buildErrorResponse(500, job->second.get_server(), job->second.get_keep_alive());
        else
            response = buildResponse(http_return_code, headers, body, job->second.get_keep_alive());
    }
    std::map<int, ClientConnection>::iterator client = _clients.find(clientFD);
    // std::cerr << "child status " << child_status << " ok " << ok << " size " << job->second.get_output().size() << std::endl;
    if (!(client == _clients.end()))
    {
        client->second.getWriteBuffer() = response;
        client->second.setWaitingForCGI(false);
    }
    job->second.set_child_pid(-1);
    // int pipeIn = job->second.get_pipe_stdin();
    // if (pipeIn != -1)
    // {
    //     close(pipeIn);
    //     _cgi_pipes.erase(pipeIn);
    //     for (std::vector<struct pollfd>::iterator pfd = _poll_fds.begin(); pfd != _poll_fds.end(); ++pfd)
    //     {
    //         if (pfd->fd == pipeIn)
    //             pfd->fd = -1;
    //     }
    // }
    // _cgi_jobs.erase(job);
    destroyCGIJob(clientFD);
}

void    PollManager::destroyCGIJob(int clientFD)
{
    std::map<int, CGI>::iterator job = _cgi_jobs.find(clientFD);
    if (job == _cgi_jobs.end())
        return ;
    int status;
    pid_t pid = job->second.get_child_pid();
    int pipe_stdin = job->second.get_pipe_stdin();
    int pipe_stdout = job->second.get_pipe_stdout();
    if (pid != -1)
    {
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        job->second.set_child_pid(-1);
    }
    if (pipe_stdin != -1)
    {
        close(pipe_stdin);
        _cgi_pipes.erase(pipe_stdin);
        for (std::vector<struct pollfd>::iterator pfd = _poll_fds.begin(); pfd != _poll_fds.end(); ++pfd)
        {
            if (pfd->fd == pipe_stdin)
                pfd->fd = -1;
        }
    }
    if (pipe_stdout != -1)
    {
        close(pipe_stdout);
        _cgi_pipes.erase(pipe_stdout);
        for (std::vector<struct pollfd>::iterator pfd = _poll_fds.begin(); pfd != _poll_fds.end(); ++pfd)
        {
            if (pfd->fd == pipe_stdout)
                pfd->fd = -1;
        }
    }
    _cgi_jobs.erase(job);
}

void    PollManager::checkCGITimeouts(time_t currentTime)
{
    std::vector<int> CGITimeouts;
    for (std::map<int, CGI>::iterator it = _cgi_jobs.begin(); it != _cgi_jobs.end(); ++it)
    {
        if (currentTime - it->second.get_start() > CGI_TIMEOUT_DURATION)
            CGITimeouts.push_back(it->first);
    }
    for (std::vector<int>::const_iterator it = CGITimeouts.begin(); it != CGITimeouts.end(); ++it)
    {
        std::map<int, CGI>::iterator job = _cgi_jobs.find(*it);
        if (job == _cgi_jobs.end())
            continue;
        std::map<int, ClientConnection>::iterator client = _clients.find(job->second.get_client_fd());
        if (client != _clients.end())
        {
            client->second.getWriteBuffer() = buildErrorResponse(504, job->second.get_server(), job->second.get_keep_alive());
            client->second.setWaitingForCGI(false);
        }
        destroyCGIJob(*it);
    }
}

// Main loop
void PollManager::run() //infinite loop that asks the OS if anyone is ready, then handles them
{
    std::cout << "Server running...\n";
    while (true) // [{fd:3, events:POLLIN, revents:0}]
    {
        checkCGITimeouts(time(NULL));
        updatePollFds();
        int ready = poll(_poll_fds.data(), _poll_fds.size(), 1000);
		// poll() sleeps until something happens, or until 1 second passes.
		// and only wakes up when is triggered when any watched fd has news
		// and will timeout after 1 second if no event on any watched fd

		// Timeout checks - kick clients that havent sent anything in 60 sec
		kickIdleClients(time(NULL), 60);
        if (ready == -1)
		{
            if (errno == EINTR)
                continue; // signal interrupted poll — just retry
            throw std::runtime_error("poll() failed");
		}
		// walk through every watched fd, one by one
        // 0	                |Skip (nothing happened)
        // POLLIN on server fd	|handleNewConnections()  - new client
        // POLLIN on client fd	|handleClientRead()      - data arrived
        // POLLOUT on client fd	|handleClientWrite()     - can send
        for (size_t i = 0; i < _poll_fds.size(); i++)
        {
            if (_poll_fds[i].fd < 0)
                continue;
			if (_poll_fds[i].revents == 0) // if nothing happened on this fd, skip it
			//events is what you ask for, revents(returned events) is what you get back
                continue; //i++
            int fd = _poll_fds[i].fd;
		 	short revents = _poll_fds[i].revents;
            std::map<int, int>::iterator it = _cgi_pipes.find(fd);
            if (it != _cgi_pipes.end())
            {
                handleCGIEvent(fd, revents);
                continue;
            }
		 	if (revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				if (!isServerFd(fd))
					closeConnection(fd); // we're done with this client.. clean up
				continue;
			}
            if (revents & POLLIN) //checks if the POLLIN bit is set inside revents
            {
				if (isServerFd(fd)) //checks if a fd is our listening sockets
                    handleNewConnections(fd); //if new client connected, accpets them
                else
                {
					handleClientRead(fd); // if existing client sent data, read it
                    continue ;
				}
			}

            if (revents & POLLOUT) // check if socket is ready to be written to, so we send the pending response
            {
				if (!isServerFd(fd) && _clients.find(fd) != _clients.end()) // checks if this is a client fd (not a listening one)
                    handleClientWrite(fd); //send a pending response
            }
        }
    }
}
