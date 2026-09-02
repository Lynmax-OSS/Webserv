// ============================================================
// WEBSERV SERVER - MAIN ENTRY POINT
// ============================================================
// Compile: make re
// Run: ./webserv configs/default.conf
// ============================================================

#include "../include/Webserv.hpp"
#include "../include/NetworkHeader/PollManager.hpp"
#include "../include/NetworkHeader/SocketManager.hpp"

int main(int argc, char **argv)
{
    try
    {
        std::string path = argc > 1 ? argv[1] : "configs/default.conf";
        std::vector<ServerConfig> configs = ConfigParser(path);
        SocketManager manager(configs);
        PollManager poll(manager, configs);   // <-- pass configs here
        poll.run();
    }
    catch(const ConfigException& e)
    {
        std::cerr << "Config error: " << e.what() << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Unknown error: " << e.what() << std::endl;
    }
    return (0);
}

// #include "../include/Webserv.hpp"
// #include "../include/ConfigHeader/ConfigParser.hpp"
// #include "../include/ParserHeader/HttpParser.hpp"
// #include <iostream>
// #include <iomanip>

// // ============================================================
// // Helper to print ServerConfig
// // ============================================================
// void printServerConfig(const ServerConfig &config)
// {
//     std::cout << "=== Server Config ===" << std::endl;
//     std::cout << "Port: " << config.port << std::endl;
//     std::cout << "Server Name: " << config.server_name << std::endl;
//     std::cout << "Root: " << config.root << std::endl;
//     std::cout << "Index files: ";
//     for (size_t i = 0; i < config.index.size(); ++i)
//         std::cout << config.index[i] << " ";
//     std::cout << std::endl;
//     std::cout << "Client max body size: " << config.client_max_body_size << std::endl;
//     std::cout << "Return code: " << config.return_code << ", URL: " << config.return_url << std::endl;
//     std::cout << "Error pages:" << std::endl;
//     for (std::map<int, std::string>::const_iterator it = config.errors.begin();
//          it != config.errors.end(); ++it)
//         std::cout << "  " << it->first << " -> " << it->second << std::endl;

//     for (size_t i = 0; i < config.locations.size(); ++i)
//     {
//         const LocationConfig &loc = config.locations[i];
//         std::cout << "  Location: " << loc.path << std::endl;
//         std::cout << "    Root: " << loc.root << std::endl;
//         std::cout << "    Allowed methods: ";
//         for (size_t j = 0; j < loc.allowed_methods.size(); ++j)
//             std::cout << loc.allowed_methods[j] << " ";
//         std::cout << std::endl;
//         std::cout << "    Autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;
//         std::cout << "    CGI ext: " << loc.cgi_extension << ", path: " << loc.cgi_path << std::endl;
//         std::cout << "    Return code: " << loc.return_code << ", URL: " << loc.return_url << std::endl;
//     }
//     std::cout << std::endl;
// }

// // ============================================================
// // Helper to print HttpRequest
// // ============================================================
// void printRequest(const HttpRequest &req)
// {
//     std::cout << "--- Request ---" << std::endl;
//     std::cout << "Method: " << req.method << std::endl;
//     std::cout << "URI: " << req.uri << std::endl;
//     std::cout << "Path: " << req.path << std::endl;
//     std::cout << "Query: " << req.query_string << std::endl;
//     std::cout << "Version: " << req.http_version << std::endl;
//     std::cout << "Keep-Alive: " << (req.keep_alive ? "yes" : "no") << std::endl;
//     std::cout << "Content-Length: " << req.content_length << std::endl;
//     std::cout << "Body: '" << req.body << "'" << std::endl;
//     std::cout << "Error Code: " << req.error_code << std::endl;
//     std::cout << "Headers:" << std::endl;
//     for (std::map<std::string, std::string>::const_iterator it = req.headers.begin();
//          it != req.headers.end(); ++it)
//         std::cout << "  " << it->first << ": " << it->second << std::endl;
//     std::cout << "----------------" << std::endl;
// }

// // ============================================================
// // Test HTTP parser with a given raw request
// // ============================================================
// void testHttpParser(const std::string &raw)
// {
//     HttpParser parser;
//     parser.feed(raw);
//     if (parser.hasError())
//     {
//         std::cout << "Parser error: " << parser.getRequest().error_code << std::endl;
//     }
//     else if (parser.isComplete())
//     {
//         printRequest(parser.getRequest());
//     }
//     else
//     {
//         std::cout << "Request incomplete (waiting for more data)" << std::endl;
//     }
//     std::cout << std::endl;
// }

// // ============================================================
// // MAIN
// // ============================================================
// int main(int argc, char **argv)
// {
//     std::string config_path = (argc > 1) ? argv[1] : "configs/default.conf";

//     // -------- Member 1: Config Parser --------
//     std::cout << "========== Config Parser Tests ==========" << std::endl;
//     try
//     {
//         std::vector<ServerConfig> configs = ConfigParser(config_path);
//         std::cout << "Config parsed successfully. Number of servers: " << configs.size() << std::endl;
//         for (size_t i = 0; i < configs.size(); ++i)
//             printServerConfig(configs[i]);
//     }
//     catch (const std::exception &e)
//     {
//         std::cerr << "Config parse error: " << e.what() << std::endl;
//         return 1;
//     }

//     // -------- Member 2: HTTP Parser --------
//     std::cout << "\n========== HTTP Parser Tests ==========" << std::endl;

//     // Test 1: Simple GET
//     testHttpParser("GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");

//     // Test 2: GET with query string
//     testHttpParser("GET /search?q=hello%20world HTTP/1.1\r\nHost: localhost\r\n\r\n");

//     // Test 3: POST with body
//     testHttpParser("POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: 11\r\n\r\nHello World");

//     // Test 4: Chunked encoding
//     testHttpParser("POST /upload HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n"
//                    "5\r\nHello\r\n6\r\n World\r\n0\r\n\r\n");

//     // Test 5: Missing Host header (should error)
//     testHttpParser("GET / HTTP/1.1\r\n\r\n");

//     // Test 6: Invalid method (should error)
//     testHttpParser("PUT / HTTP/1.1\r\nHost: localhost\r\n\r\n");

//     // Test 7: Invalid HTTP version (should error)
//     testHttpParser("GET / HTTP/0.9\r\nHost: localhost\r\n\r\n");

//     // Test 8: Keep-alive detection
//     testHttpParser("GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n");

//     // Test 9: Pipelining (two requests in one buffer)
//     {
//         HttpParser parser;
//         std::string pipeline =
//             "GET /page1.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
//             "GET /page2.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
//         parser.feed(pipeline);
//         std::cout << "Pipelining test:" << std::endl;
//         while (parser.isComplete())
//         {
//             printRequest(parser.getRequest());
//             parser.reset(true);   // keep leftover buffer
//             parser.feed("");      // process any remaining data
//         }
//         std::cout << std::endl;
//     }

//     // Test 10: Body too large (set limit)
//     {
//         HttpParser parser;
//         parser.setMaxBodySize(10);
//         std::string big_body(100, 'X');
//         parser.feed("POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: 100\r\n\r\n" + big_body);
//         if (parser.hasError())
//             std::cout << "Body too large -> error code: " << parser.getRequest().error_code << std::endl;
//         else
//             std::cout << "Parser did not detect body size limit!" << std::endl;
//         std::cout << std::endl;
//     }

//     std::cout << "All tests completed." << std::endl;
//     return 0;
// }
