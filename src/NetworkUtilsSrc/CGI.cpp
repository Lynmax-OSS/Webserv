/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yosherau <yosherau@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 03:02:39 by yosherau          #+#    #+#             */
/*   Updated: 2026/09/24 12:59:47 by yosherau         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/NetworkUtilsHeader/CGI.hpp"
# include "../../include/NetworkUtilsHeader/ResponseUtils.hpp"
# include <sys/stat.h>
# include <sys/wait.h>
# include <unistd.h>
# include <sstream>
# include <cstdlib>

std::string handleCGI(const HttpRequest &req, const ServerConfig *server, const LocationConfig *location, const std::string &scriptPath)
{
	std::cerr << "CGI scriptPath = [" << scriptPath << "]" << std::endl;
	struct stat st;
	if (stat(scriptPath.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
		return buildErrorResponse(404, server, req.keep_alive);
	if (access(scriptPath.c_str(), R_OK) != 0)
		return (buildErrorResponse(403, server, req.keep_alive));
	std::string output;
    if (!runCGI(location->cgi_path, scriptPath, req, output))
		return (buildErrorResponse(500, server, req.keep_alive));
	int status;
	std::string	contentType, body;
	parseCgiOutput(output, status, contentType, body);
	std::map<std::string, std::string> headers;
	headers["Content-Type"] = contentType;
	return (buildResponse(status, headers, body, req.keep_alive));
}

static void parseCgiOutput(const std::string &output, int &statusCode,
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

bool	runCGI(std::string cgi_path, std::string script_path, const HttpRequest &req, std::string &output)
{
	(void)cgi_path;
	(void)script_path;
	(void)output;
	(void)req;
	int	parentFD[2];
	int	childFD[2];

	if (pipe(parentFD) == -1)
		return (false);
	if (pipe(childFD) == -1)
	{
		close(parentFD[0]);
		close(parentFD[1]);
		return (false);
	}
	std::vector<std::string>	env;
	std::string					contentType;
	std::map<std::string, std::string>::const_iterator it = req.headers.find("Content-Type");
	if (it != req.headers.end())
		contentType = it->second;
	env.push_back("REQUEST_METHOD=" + req.method);
	env.push_back("QUERY_STRING=" + req.query_string);
	env.push_back("SCRIPT_NAME=" + req.path);
	env.push_back("SCRIPT_FILENAME=" + script_path);
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SEVER_PROTOCOL=" + req.http_version);
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
	argv[1] = const_cast<char *>(script_path.c_str());
	argv[2] = NULL;
	
	pid_t	pid = fork();
	if (pid < 0)
	{
		close(parentFD[0]);
		close(parentFD[1]);
		close(childFD[0]);
		close(childFD[1]);
		return (false);
	}
	if (pid == 0)
	{
		dup2(parentFD[0], STDIN_FILENO);
		dup2(childFD[1], STDOUT_FILENO);
		close(parentFD[0]);
		close(parentFD[1]);
		close(childFD[0]);
		close(childFD[1]);
		execve(cgi_path.c_str(), argv, &env_array[0]);
		std::exit(EXIT_FAILURE);
	}

	close(parentFD[0]);
	close(childFD[1]);
	
	size_t	written = 0;
	while (written < req.body.size())
	{
		size_t n = write(parentFD[1], req.body.c_str() + written, req.body.size() - written);
		if (n <= 0)
			break;
		written += n;
	}
	close(parentFD[1]);
	char	buf[4096];
	size_t	n;
	while ((n = read(childFD[0], buf, sizeof(buf))) > 0)
		output.append(buf, n);
	close(childFD[0]);

	int	status;
	waitpid(pid, &status, 0);
	return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}