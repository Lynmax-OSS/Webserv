#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <map>
#include <string>

struct HttpRequest {
    // Basic request parts
    std::string method;          // "GET", "POST", "DELETE"
    std::string uri;             // Original URI "/index.html?name=john"
    std::string path;            // Decoded path "/index.html"
    std::string query_string;    // "name=john"
    std::string http_version;    // "HTTP/1.1"
    
    // Headers (key -> value)
    std::map<std::string, std::string> headers;
    
    // Body content
    std::string body;
    
    // Flags
    bool keep_alive;             // true if Connection: keep-alive
    int content_length;          // -1 if not specified
    
    // Error handling
    int error_code;              // 0 = OK, 400 = Bad Request, etc.
    
    // Constructor with defaults
    HttpRequest() 
        : keep_alive(false), 
          content_length(-1), 
          error_code(0) {}
};

#endif