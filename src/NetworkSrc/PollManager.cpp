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
	if (this != &other)
	{
		_poll_fds = other._poll_fds;
	}
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
	_clients[client_fd] = ClientConnection(client_fd); //q - create clientconnection
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
	// //Below this point is where server and client interactions begin
	// std::ifstream file("./www/index.html");// this is hardcoded 
	// std::string body;
	// if (file.is_open())
	// {
	// 	std::string line;
	// 	while (std::getline(file, line))
	// 	{
	// 		body += line;
	// 		body += "\n";
	// 	}
	// }
	// else
	// 	body = "<h1>404 Not Found</h1>"; // there is no proper 404 page so this is shrimple (can implement your own)

	// std::ostringstream response;
	// response << "HTTP/1.1 200 OK\r\n"
	// 		 << "Content-Type: text/html\r\n"
	// 		 << "Content-Length: " << body.size() << "\r\n"
	// 		 << "Connection: close\r\n"
	// 		 << "\r\n"
	// 		 << body;

	// std::string res = response.str();
	// write(client_fd, res.c_str(), res.size());
	// close(client_fd);
	// removeFd(client_fd);
	
	//below this is edited by qhaby
    // ✅ Feed data to parser
    ClientConnection& conn = _clients[client_fd];
    conn.getParser().feed(std::string(buf, bytes));
    conn.updateActivity();  // Reset timeout timer
    
    // ✅ Check if request is complete
    if (conn.getParser().isComplete()) {
        const HttpRequest& req = conn.getParser().getRequest();
        
        // Check for parsing errors
        if (req.error_code != 0)
		{
            // Send error response (or let Member 3 handle)
			std::ostringstream error_stream;
			error_stream << "HTTP/1.1 " << req.error_code << " Error\r\n"
						<< "Content-Type: text/html\r\n"
						<< "Connection: close\r\n"
						<< "\r\n"
						<< "<h1>Error " << req.error_code << "</h1>";
			std::string error = error_stream.str();
			write(client_fd, error.c_str(), error.size());
        } else {
			// ✅ Valid request - pass to Member 3
			std::cout << "✅ Received: " << req.method << " " << req.path << std::endl;

			// ---------- TEMPORARY HANDLER (for testing) ----------
			if (req.method == "GET") {
				// Hardcoded root for now (you can change later to use config)
				std::string base_path = "./www";
				std::string file_path = base_path + req.path;

				// If the path ends with '/', append index.html
				if (!file_path.empty() && file_path[file_path.size() - 1] == '/')
					file_path += "index.html";

				std::ifstream file(file_path.c_str(), std::ios::binary);
				if (file.is_open()) {
					std::string body((std::istreambuf_iterator<char>(file)),
									std::istreambuf_iterator<char>());
					file.close();

					// Determine basic content type
					std::string content_type = "text/html";
					if (file_path.find(".css") != std::string::npos)
						content_type = "text/css";
					else if (file_path.find(".js") != std::string::npos)
						content_type = "application/javascript";
					else if (file_path.find(".png") != std::string::npos)
						content_type = "image/png";
					else if (file_path.find(".jpg") != std::string::npos ||
							file_path.find(".jpeg") != std::string::npos)
						content_type = "image/jpeg";
					else if (file_path.find(".gif") != std::string::npos)
						content_type = "image/gif";
					// ... add more if needed

					// Build HTTP response
					std::ostringstream response;
					response << "HTTP/1.1 200 OK\r\n"
							<< "Content-Type: " << content_type << "\r\n"
							<< "Content-Length: " << body.size() << "\r\n"
							<< "Connection: keep-alive\r\n"
							<< "\r\n"
							<< body;

					conn.getWriteBuffer() = response.str();
				} else {
					// File not found → 404
					std::ostringstream response;
					response << "HTTP/1.1 404 Not Found\r\n"
							<< "Content-Type: text/html\r\n"
							<< "Content-Length: 0\r\n"
							<< "Connection: close\r\n"
							<< "\r\n";
					conn.getWriteBuffer() = response.str();
					conn.setKeepAlive(false); // close after 404
				}
			} else {
				// Only GET is handled for now – return 405 for other methods
				std::ostringstream response;
				response << "HTTP/1.1 405 Method Not Allowed\r\n"
						<< "Content-Type: text/html\r\n"
						<< "Content-Length: 0\r\n"
						<< "Connection: close\r\n"
						<< "\r\n";
				conn.getWriteBuffer() = response.str();
				conn.setKeepAlive(false);
			}
            
            // Send response from write buffer
            if (!conn.getWriteBuffer().empty()) 
			{
                write(client_fd, conn.getWriteBuffer().c_str(), conn.getWriteBuffer().size());
                conn.getWriteBuffer().clear();
            }
        }
        
        // ✅ Handle keep-alive
        if (req.keep_alive) {
            conn.setKeepAlive(true);
            conn.getParser().reset();  // Ready for next request
            // Keep socket open!
        } else {
            conn.setKeepAlive(false);
			//--------------------------------------
			//✅so when the poll() is still watching that file descriptor until you call removeFd()
			// you close() it, so the FD becomes invalid
			//then removeFd() tries to remove it, which might;
			//  access invalid memory, 
			// remove the wrong FD if the number was reused and 
			// cause undefined behaviour
			// so I swapped the removeFd() ad close() positions
			removeFd(client_fd);
            close(client_fd);
            _clients.erase(client_fd);
        }
    }
}

void PollManager::run()
{
	std::cout << "Server running...\n";
	while (true)
	{
		int ready = poll(_poll_fds.data(), _poll_fds.size(), 1000);
		
        // -q- ✅ Check for timeouts on clients
        time_t now = time(NULL);
        for (std::map<int, ClientConnection>::iterator it = _clients.begin(); it != _clients.end();) 
		{
			if (it->second.isTimeout(now, 60)) 
			{  // 60 second timeout
				std::cout << "Timeout on fd " << it->first << std::endl;
				close(it->first);
				removeFd(it->first);
				_clients.erase(it++);
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
