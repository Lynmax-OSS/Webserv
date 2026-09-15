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

#include "../../include/NetworkUtilsHeader/ResponseUtils.hpp"
#include <fstream>
#include <sstream>

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

