#include "../../include/Webserv.hpp"
#include "../../include/NetworkHeader/PollManager.hpp"
#include "../../include/NetworkUtilsHeader/ResponseUtils.hpp"

// parsed request is ready, check if ok, then hands off, sends back, and resets or closes
void PollManager::processRequest(int fd)
{
    ClientConnection& conn = _clients[fd];
    const HttpRequest& req = conn.getParser().getRequest();

    if (req.error_code != 0)
    {
        // Build error response for parse error
        std::ostringstream error_stream;
        error_stream << "HTTP/1.1 " << req.error_code << " Error\r\n"
                    << "Content-Type: text/html\r\n"
                    << "Connection: close\r\n"
                    << "\r\n"
                    << "<h1>Error " << req.error_code << "</h1>";
        conn.getWriteBuffer() = error_stream.str(); // turn the strBuilder into str and store in cleint's write buffer
        conn.setKeepAlive(false); // close after error
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
        conn.getParser().reset(true); // keep leftover buffer
        conn.getParser().feed(""); // process any pipelined data
    }
    else
    {
        // If not keep-alive, close connection
        closeConnection(fd);
    }
}

// void PollManager::handleRequest(const HttpRequest& req, const std::vector<ServerConfig>& configs, std::string& response)
// {
//     const ServerConfig* server = NULL;
//     {
//         std::string host;
//         std::map<std::string, std::string>::const_iterator hostIt = req.headers.find("Host");
//         if (hostIt != req.headers.end())
//             host = hostIt->second;

//         int port = 8080;
//         size_t colonPos = host.find(':');
//         if (colonPos != std::string::npos)
//             port = std::atoi(host.substr(colonPos + 1).c_str());
//         for (size_t i = 0; i < configs.size(); ++i)
//         {
//             if (configs[i].port == port)
//             {
//                 server = &configs[i];
//                 break;
//             }
//         }
//         if (server == NULL && !configs.empty())
//             server = &configs[0]; // fallback to first server if no match
//     }
//     if (server == NULL)
//     {
//         response = buildErrorResponse(500, NULL, false);
//         return ;
//     }

//     const LocationConfig* location = NULL;
//     {
//         size_t longestMatch = 0;
//         for (size_t i = 0; i < server->locations.size(); ++i)
//         {
//             const LocationConfig& loc = server->locations[i];
//             if (req.path.compare(0, loc.path.size(), loc.path) == 0
//                 && loc.path.size() > longestMatch)
//             {
//                 location = &loc;
//                 longestMatch = loc.path.size();
//             }
//         }
//     }
//     if (location != NULL && !location->allowed_methods.empty())
//     {
//         bool allowed = false;
//         for (size_t i = 0; i < location->allowed_methods.size(); ++i)
//         {
//             if (req.method == location->allowed_methods[i])
//             {
//                 allowed = true;
//                 break;
//             }
//         }
//         if (!allowed)
//         {
//             response = buildErrorResponse(405, server, false);
//             return ;
//         }
//     }
//     if (location != NULL && location->return_code != 0)
//     {
//         std::map<std::string, std::string> headers;
//         headers["Location"] = location->return_url;
//         response = buildResponse(location->return_code, headers, "", req.keep_alive);
//         return ;
//     }
//     if (req.method == "GET")
//     {
//         reponse = buildErrorResponse(500, server, req.keep_alive);
//         return ;
//     }
// }
