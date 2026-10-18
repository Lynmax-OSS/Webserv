#include "../../include/Webserv.hpp"
#include "../../include/NetworkHeader/PollManager.hpp"
#include "../../include/NetworkUtilsHeader/ResponseUtils.hpp"
#include "../../include/NetworkUtilsHeader/CGI.hpp"
#include <dirent.h>
#include <algorithm>

// Process one complete request
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

// Main handler (to be implemented by Member 3)
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
    struct stat path_stat;
    std::string filePath;
	
    const ServerConfig* server = NULL;
    {
        std::string host;
        std::map<std::string, std::string>::const_iterator hostIt = req.headers.find("Host");
        if (hostIt != req.headers.end())
            host = hostIt->second;
    
        int port = 8080;
        size_t colonPos = host.find(':');
        if (colonPos != std::string::npos)
            port = std::atoi(host.substr(colonPos + 1).c_str());
        for (size_t i = 0; i < configs.size(); ++i)
        {
            if (configs[i].port == port)
            {
                server = &configs[i];
                break;
            }
        }
        if (server == NULL && !configs.empty())
            server = &configs[0]; // fallback to first server if no match
    }
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

    if (!location->cgi_extension.empty() && location->cgi_extension == getExtension(req.uri))
    {
        response = handleCGI(req, server, location, resolvePath(server, location, req));
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
        if (req.has_content_length && req.content_length > server->client_max_body_size)
        {
            response = buildErrorResponse(413, server, false);
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
            response = buildErrorResponse(409, server, req.keep_alive);
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
