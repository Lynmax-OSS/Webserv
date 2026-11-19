/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yosherau <yosherau@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 03:02:39 by yosherau          #+#    #+#             */
/*   Updated: 2026/11/18 14:50:31 by yosherau         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/NetworkUtilsHeader/CGI.hpp"
# include "../../include/NetworkUtilsHeader/ResponseUtils.hpp"
# include "../../include/NetworkHeader/PollManager.hpp"

# include <sys/stat.h>
# include <sys/wait.h>
# include <unistd.h>
# include <sstream>
# include <cstdlib>
# include <fcntl.h>

CGI::CGI():
	child_pid(-1), pipe_stdin(-1), pipe_stdout(-1), output(""), client_fd(-1), keep_alive(false), start(0), body_written(0), server(NULL)
{

}

pid_t	CGI::get_child_pid(void) const
{
	return (child_pid);
}

void	CGI::set_child_pid(pid_t new_pid)
{
	this->child_pid = new_pid;
}

int		CGI::get_pipe_stdin(void) const
{
	return (pipe_stdin);
}

void	CGI::set_pipe_stdin(int pipe_stdin)
{
	this->pipe_stdin = pipe_stdin;
}

int		CGI::get_pipe_stdout(void) const
{
	return (pipe_stdout);
}

void	CGI::set_pipe_stdout(int pipe_stdout)
{
	this->pipe_stdout = pipe_stdout;
}

const std::string	&CGI::get_output(void) const
{
	return (output);
}

void	CGI::set_output(std::string new_output)
{
	this->output = new_output;
}

const std::string	&CGI::get_body(void) const
{
	return (body);
}

void    CGI::append_output(const char *buffer, ssize_t length)
{
	if (length < 0)
		return ;
	this->output.append(buffer, length);
}

void	CGI::set_body(std::string new_body)
{
	this->body = new_body;
}

int	CGI::get_client_fd(void) const
{
	return (client_fd);
}

void	CGI::set_client_fd(int new_fd)
{
	this->client_fd = new_fd;
}

bool	CGI::get_keep_alive(void) const
{
	return (keep_alive);
}

void	CGI::set_keep_alive(bool new_keep_alive)
{
	this->keep_alive = new_keep_alive;
}

time_t	CGI::get_start(void) const
{
	return (start);
}

void	CGI::set_start(time_t new_start)
{
	this->start = new_start;
}

size_t	CGI::get_body_written(void) const
{
	return (body_written);
}

void	CGI::set_body_written(size_t written)
{
	this->body_written = written;
}

const	ServerConfig *CGI::get_server(void) const
{
	return (server);
}

void	CGI::set_server(const ServerConfig *new_server)
{
	this->server = new_server;
}


std::string PollManager::handleCGI(const HttpRequest &req, const ServerConfig *server, const LocationConfig *location, const std::string &scriptPath, ClientConnection &conn)
{
	std::cerr << "CGI scriptPath = [" << scriptPath << "]" << std::endl;
	struct stat st;
	if (stat(scriptPath.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
		return buildErrorResponse(404, server, req.keep_alive);
	if (access(scriptPath.c_str(), R_OK) != 0)
		return (buildErrorResponse(403, server, req.keep_alive));
	if (static_cast<size_t>(req.content_length) > server->client_max_body_size)
		return (buildErrorResponse(413, server, req.keep_alive));
    if (!runCGI(location->cgi_path, scriptPath, req, conn.getFd(), server))
		return (buildErrorResponse(500, server, req.keep_alive));
	conn.setWaitingForCGI(true);
	return "";
}

void parseCgiOutput(const std::string &output, int &statusCode,
                           std::string &contentType, std::string &body)
{
    statusCode = 200;
    contentType = "text/html";
    size_t sep = output.find("\r\n\r\n");
    size_t sepLen = 4;
    if (sep == std::string::npos)
    {
        sep = output.find("\n\n");
        sepLen = 2;
    }
    if (sep == std::string::npos)
    {
        body = output;
        return;
    }
    std::string headers = output.substr(0, sep);
    body = output.substr(sep + sepLen);

    std::istringstream hs(headers);
    std::string line;
    while (std::getline(hs, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.compare(0, 13, "Content-Type:") == 0)
        {
            size_t v = line.find_first_not_of(" \t", 13);
            if (v != std::string::npos)
                contentType = line.substr(v);
        }
        else if (line.compare(0, 7, "Status:") == 0)
            statusCode = std::atoi(line.c_str() + 7);
    }
}

bool	PollManager::runCGI(std::string cgi_path, std::string script_path, const HttpRequest &req, int client_fd, const ServerConfig *server)
{
	int	to_child[2];
	int	from_child[2];

	if (pipe(to_child) == -1)
		return (false);
	if (pipe(from_child) == -1)
	{
		close(to_child[0]);
		close(to_child[1]);
		return (false);
	}
	std::vector<std::string>	env;
	std::string					contentType;
	std::ostringstream oss;
	oss << server->port;
	std::string	serverPort = oss.str();
	std::map<std::string, std::string>::const_iterator it = req.headers.find("Content-Type");
	if (it != req.headers.end())
		contentType = it->second;

	size_t  slash = script_path.rfind('/');
	std::string	file;
	std::string	directory;
	if (slash != std::string::npos)
	{
		directory = script_path.substr(0, slash);
		file = script_path.substr(slash + 1);
	}
	else
	{
		directory = ".";
		file = script_path;
	}
	env.push_back("REQUEST_METHOD=" + req.method);
	env.push_back("QUERY_STRING=" + req.query_string);
	env.push_back("SCRIPT_NAME=" + req.path);
	env.push_back("SCRIPT_FILENAME=" + file);
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_PROTOCOL=" + req.http_version);
	env.push_back("SERVER_NAME=" + server->server_name);
	for (std::map<std::string, std::string>::const_iterator headers = req.headers.begin(); headers != req.headers.end(); ++headers)
	{
		std::string key = headers->first;
		for (std::size_t i = 0; i < key.length(); ++i)
		{
			if (key[i] == '-')
				key[i] = '_';
			else
				key[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(key[i])));
		}
		env.push_back("HTTP_" + key + "=" + headers->second);
	}
	env.push_back("SERVER_PORT=" + serverPort);
	env.push_back("SERVER_SOFTWARE=webserv/1.0");
	env.push_back("PATH_INFO=" + req.path);


	if (!req.body.empty())
	{
		std::ostringstream len;
		len << req.body.size();
		env.push_back("CONTENT_LENGTH=" + len.str());
		env.push_back("CONTENT_TYPE=" + contentType);
	}
	std::vector<char *> env_array;
	for (size_t index = 0; index < env.size(); index++)
		env_array.push_back(const_cast<char *>(env[index].c_str()));
	env_array.push_back(NULL);

	char *argv[3];
	argv[0] = const_cast<char *>(cgi_path.c_str());
	argv[1] = const_cast<char *>(file.c_str());
	argv[2] = NULL;

	pid_t	pid = fork();
	if (pid < 0)
	{
		close(to_child[0]);
		close(to_child[1]);
		close(from_child[0]);
		close(from_child[1]);
		return (false);
	}
	if (pid == 0)
	{
		dup2(to_child[0], STDIN_FILENO);
		dup2(from_child[1], STDOUT_FILENO);
		close(to_child[0]);
		close(to_child[1]);
		close(from_child[0]);
		close(from_child[1]);

		for (std::vector<pollfd>::iterator pfd = _poll_fds.begin(); pfd != _poll_fds.end(); ++pfd)
		{
			if (pfd->fd > 2)
				close(pfd->fd);
		}
		if (chdir(directory.c_str()) != 0)
			_exit(EXIT_FAILURE);
		execve(cgi_path.c_str(), argv, &env_array[0]);
		_exit(EXIT_FAILURE);
	}

	close(to_child[0]);
	close(from_child[1]);
	fcntl(to_child[1],  F_SETFL, O_NONBLOCK);
	fcntl(from_child[0], F_SETFL, O_NONBLOCK);

	CGI &job = _cgi_jobs[client_fd];
	job.set_child_pid(pid);
	job.set_pipe_stdout(from_child[0]);
	job.set_body(req.body);
	job.set_client_fd(client_fd);
	job.set_keep_alive(req.keep_alive);
	job.set_start(time(NULL));
	job.set_server(server);
	_cgi_pipes[from_child[0]] = client_fd;
	addFd(from_child[0], POLLIN);

	if (!req.body.empty())
	{
		job.set_pipe_stdin(to_child[1]);
		_cgi_pipes[to_child[1]] = client_fd;
		addFd(to_child[1], POLLOUT);
	}
	else
		close(to_child[1]);

	return (true);
}