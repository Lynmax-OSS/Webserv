/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseUtils.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yosherau <yosherau@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/11 19:04:39 by yosherau          #+#    #+#             */
/*   Updated: 2026/09/11 20:27:05 by yosherau         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/NetworkUtilsHeader/ResponseUtils.hpp"
# include <fstream>
# include <sstream>
# include <sys/stat.h>

std::string getReasonPhrase(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 505: return "HTTP Version Not Supported";
        default: return "Unknown Status";
    }
}

std::string getMimeType(const std::string &path)
{
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream";
    }
    std::string extension = path.substr(dotPos);
    if (extension == ".html" || extension == ".htm") return "text/html";
    if (extension == ".css") return "text/css";
    if (extension == ".js") return "application/javascript";
    if (extension == ".json") return "application/json";
    if (extension == ".txt") return "text/plain";
    if (extension == ".png") return "image/png";
    if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
    if (extension == ".gif") return "image/gif";
    if (extension == ".svg") return "image/svg+xml";
    if (extension == ".ico") return "image/x-icon";
    if (extension == ".pdf") return "application/pdf";
    return "application/octet-stream";
}

bool    readFileToString(const std::string &path, std::string &out)
{
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    std::ostringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

std::string buildResponse(int status, const std::map<std::string, std::string>& extraHeaders, const std::string &body, bool keepAlive)
{
    std::ostringstream oss;

    oss << "HTTP/1.1 " << status << " " << getReasonPhrase(status) << "\r\n";
    
    for (std::map<std::string, std::string>::const_iterator it = extraHeaders.begin(); it != extraHeaders.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }
    
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
    oss << "\r\n";
    oss << body;
    return (oss.str());
}

std::string buildErrorResponse(int status, const ServerConfig *server, bool keepAlive)
{
    std::string body;
    bool        usedCustomPage = false;

    if (server != NULL)
    {
        std::map<int, std::string>::const_iterator it = server->errors.find(status);
        if (it != server->errors.end())
        {
            if (readFileToString(it->second, body))
                usedCustomPage = true;
        }
    }
    if (!usedCustomPage)
    {
        std::ostringstream ss;
        ss << "<html><head><title>" << status << " " << getReasonPhrase(status) << "</title></head><body><h1>" << status
        << " " << getReasonPhrase(status) << "</h1></body></html>";
        body = ss.str();
    }
    
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "text/html";
    return buildResponse(status, headers, body, keepAlive);
}

std::string buildAutoIndexPage(const std::string &urlPath, const std::string &dirPath, const std::vector<std::string> &names)
{
    std::ostringstream  oss;
    std::string         base = urlPath;

    if (base[base.length() - 1] != '/')
            base += "/";
    oss << "<html><head><title>Index of " << urlPath << "</title></head>\n"
        << "<body><h1>Index of " << urlPath << "</h1><hr><pre>\n";

    for (size_t index = 0; index < names.size(); ++index)
    {
        std::string name = names[index];
        std::string fullPath = dirPath;

        if (fullPath[fullPath.length() - 1] != '/')
            fullPath += "/";
        fullPath += name;
        struct stat st;
        
        if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
            name += "/";
        oss << "<a href=\"" << base << name << "\">" << name << "</a>\n";
    }
    oss << "</pre><hr></body></html>\n";
    return (oss.str());
}

bool    hasParentTraversal(const std::string &path)
{
    size_t  pos = path.find("/");
    while (pos != std::string::npos)
    {
        if (path.compare(pos + 1, 2, "..") == 0 && (path[pos + 1 + 2] == '/' || path[pos + 1 + 2] == '\0'))
            return (true);
        pos = path.find("/", pos + 1);
    }
	if (path.compare("..") == 0)
		return (true);
	return (false);
}

std::string resolvePath(const ServerConfig *server, const LocationConfig *location, const HttpRequest& req)
{
    std::string path = server->root;
    std::string locationPath = "";
    
    if (location)
    {
        if (!location->root.empty())
            path = location->root;
        locationPath = location->path;
    }
    std::string remainder = req.path;
    if (!locationPath.empty() && req.path.compare(0, locationPath.size(), locationPath) == 0 && (req.path.size() == locationPath.size() || req.path[locationPath.size()] == '/'))
        remainder = req.path.substr(locationPath.size());
    if (!path.empty() && path[path.size() - 1] == '/')
        path.erase(path.size() - 1);
    if (!remainder.empty() && remainder[0] != '/')
        remainder = "/" + remainder;
    path = path + remainder;
    return (path);
}

std::string getExtension(const std::string &path)
{
    std::string path = "/cgi-bin/hello.py?name=yogi";
	size_t location = path.find('?');
	std::string filteredPath = path.substr(0, location);
	size_t dotLocation = filteredPath.find_last_of('.');
	size_t slashLocation = filteredPath.find_last_of('/');

	if ((dotLocation == std::string::npos) || (slashLocation != std::string::npos && dotLocation < slashLocation))
		std::cout << "" << std::endl;
	std::cout << filteredPath.substr(dotLocation) << std::endl; 
}
