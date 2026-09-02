# Integration Guide for Member 3 – Request Handling & HTTP Response

## Your Task

You are responsible for implementing the actual request handling – taking a parsed `HttpRequest` and producing a valid HTTP response. This includes:

- Routing requests to the correct `ServerConfig` and `LocationConfig`.
- Serving static files (GET), handling uploads (POST), DELETE, and directory listing (autoindex).
- Executing CGI scripts.
- Implementing custom error pages, redirects, and proper HTTP status codes.
- Building the complete HTTP response (status line, headers, body).

The server infrastructure (config parsing, socket handling, poll loop, HTTP request parser, and connection management) is already done. Your main entry point is the `PollManager::handleRequest` function.

---

## What’s Already Done (Members 1 & 2)

- **Config parsing** – `ServerConfig` and `LocationConfig` structs with all necessary fields (root, index, locations, allowed_methods, autoindex, CGI, return, error pages, etc.).
- **Socket & event loop** – Non-blocking sockets, `poll()`-based loop, connection handling, keep-alive, timeout management.
- **HTTP request parser** – `HttpParser` produces a complete `HttpRequest` (method, path, headers, body, query string, keep-alive flag, error code).

The server already calls your handler with a fully parsed request and the full config list. **You do not need to worry about sockets, parsing, or non-blocking writes** – only produce the response string.

---

## Your Main Function: `PollManager::handleRequest`

Location: `src/NetworkSrc/PollManager.cpp`

```cpp
void PollManager::handleRequest(const HttpRequest& req,
                                const std::vector<ServerConfig>& configs,
                                std::string& response)
```
______________________________________________________________________________________

# Inputs:
> req – the parsed HttpRequest (see struct below).

> configs – all server configurations parsed from the config file.

> response – a string you must fill with the complete HTTP response.

# Output:
Fill response with the full HTTP response, e.g.:

    HTTP/1.1 200 OK\r\n
    Content-Type: text/html\r\n
    Content-Length: 1234\r\n
    Connection: keep-alive\r\n
    \r\n
    <html>...</html>
______________________________________________________________________________________

# Structures You Need:

```cpp
>> HttpRequest (from include/ParserHeader/HttpRequest.hpp)

    struct HttpRequest {
        std::string method;         // "GET", "POST", "DELETE"
        std::string uri;            // Original URI, e.g., "/index.html?name=john"
        std::string path;           // Decoded path, e.g., "/index.html"
        std::string query_string;   // "name=john"
        std::string http_version;   // "HTTP/1.1"
        std::map<std::string, std::string> headers; // All request headers
        std::string body;           // Request body (already decoded)
        bool keep_alive;            // true if Connection: keep-alive
        int content_length;         // -1 if not specified
        int error_code;             // 0 = OK (parsing errors are handled before this)
    };

>> ServerConfig and LocationConfig (from include/ConfigHeader/)

    struct ServerConfig {
        int port;
        std::string host;
        std::string server_name;
        std::string root;
        std::vector<std::string> index;
        std::vector<LocationConfig> locations;
        std::map<int, std::string> errors;   // error code -> file path
        size_t client_max_body_size;
        std::string return_url;
        int return_code;
    };

    struct LocationConfig {
        std::string path;                     // location prefix
        std::string root;
        std::vector<std::string> allowed_methods;
        bool autoindex;
        std::string cgi_extension;
        std::string cgi_path;
        int return_code;
        std::string return_url;
    };
```
______________________________________________________________________________________

## Step-by-Step Implementation Guidance

1. Find_the_Correct_ServerConfig

> If there is only one server (common in tests), use configs[0].

> For multiple servers, match by the Host header (includes port) or by the port the request came in on (you can get the port from the server socket, but simpler to use Host). Example:

```cpp
    const ServerConfig* server = NULL;
    std::string host = req.headers["Host"];   // "localhost:8080"
    // parse port from host (if present)
    int port = 8080; // default
    size_t colon = host.find(':');
    if (colon != std::string::npos)
        port = atoi(host.substr(colon + 1).c_str());

    for (size_t i = 0; i < configs.size(); ++i) {
        if (configs[i].port == port) {
            server = &configs[i];
            break;
        }
    }
    if (server == NULL)
        server = &configs[0]; // fallback
```

2. Find_the_Matching_LocationConfig

> Iterate through server->locations and find the longest matching prefix with req.path.

> If a LocationConfig has an empty root, inherit from server->root.

```cpp
    const LocationConfig* best = NULL;
    size_t best_len = 0;
    for (size_t i = 0; i < server->locations.size(); ++i) {
        const LocationConfig& loc = server->locations[i];
        if (req.path.compare(0, loc.path.size(), loc.path) == 0) {
            if (loc.path.size() > best_len) {
                best = &loc;
                best_len = loc.path.size();
            }
        }
    }
```

3. Check_allowed_methods

> If the LocationConfig has a list of allowed_methods, verify req.method is in that list.

> If not, return 405 Method Not Allowed.

```cpp
    bool method_allowed = true;
    if (best && !best->allowed_methods.empty()) {
        method_allowed = false;
        for (size_t i = 0; i < best->allowed_methods.size(); ++i)
            if (req.method == best->allowed_methods[i]) {
                method_allowed = true;
                break;
            }
    }
    if (!method_allowed) {
        response = buildErrorResponse(server, 405, req.keep_alive);
        return;
    }
```

4. Handle_Redirects_(return directive)

> If best has a return_code (e.g., 301) and return_url, send a response with that status and a Location header.

```cpp
    if (best && best->return_code != 0) {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << best->return_code << " Redirect\r\n"
            << "Location: " << best->return_url << "\r\n"
            << "Content-Length: 0\r\n"
            << "Connection: " << (req.keep_alive ? "keep-alive" : "close") << "\r\n"
            << "\r\n";
        response = oss.str();
        return;
    }
```

5. Route_Based_on_Method

## GET (static file / directory listing)
> Build the file path: root + req.path. If req.path ends with /, append the first index file (e.g., server->index[0]).

> If the target is a directory and autoindex is on, generate an HTML listing.

> If the file exists, read it, determine Content-Type from extension, and send 200 OK.

> If not found, send 404 Not Found using a custom error page if available.

## POST (upload)
> Save the request body to a file inside the appropriate root (e.g., ./uploads).

> Use the filename from Content-Disposition header if present, or generate a name.

> Return 201 Created or 200 OK.

## DELETE
> Remove the file specified by req.path (relative to the root).

> Return 204 No Content or 200 OK.

## CGI (e.g., /cgi-bin/script.py)
> If the location has cgi_extension and req.path ends with that extension:
    
    ~ Use pipe(), fork(), execve() to run cgi_path (e.g., /usr/bin/python3).
    
    ~ Pass environment: QUERY_STRING, REQUEST_METHOD, CONTENT_LENGTH, CONTENT_TYPE, etc.
    
    ~ Read the script’s stdout; this is the response body (may include headers – you may need to parse them).
    
    ~ Return the output as the response.


6.Error_Pages

> For errors like 404, 403, 500, check server->errors for a custom page:

```cpp
    std::map<int, std::string>::const_iterator it = server->errors.find(status);
    if (it != server->errors.end()) {
        // read file it->second and use as body
    } else {
        // use simple default message
    }
```
7.Build_the_Response

> Always set:
    
    ~ Content-Type (if body is present)
    
    ~ Content-Length
    
    ~ Connection (based on req.keep_alive – but if you decide to close, set "close")
    
    ~ Status line with reason phrase (200 OK, 404 Not Found, etc.)

```cpp
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << " " << reason << "\r\n"
        << "Content-Type: " << content_type << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: " << (req.keep_alive ? "keep-alive" : "close") << "\r\n"
        << "\r\n"
        << body;
    response = oss.str();
```
______________________________________________________________________________________

## Important_Notes

Non-blocking sockets: The server manages writes. You only fill response; the event loop sends it.

Keep-alive: If you set Connection: keep-alive, the parser will be reset and the connection stays open. If you want to close, set "close".

Pipelining: Multiple requests may be processed in one read; you just handle each sequentially.

Do not touch the event loop, parser, or socket code.
______________________________________________________________________________________

# Testing Your Implementation
> Compile: make re

> Run: ./webserv configs/default.conf

> Test endpoints:
    ~ curl -v http://localhost:8080/ → serves index.html

    ~ curl -v http://localhost:8080/images/logo.png → serves from /images root
    
    ~ curl -v -X POST -F "file=@test.txt" http://localhost:8080/upload → uploads file
    
    ~ curl -v -X DELETE http://localhost:8080/uploads/test.txt → deletes file
    
    ~ curl -v http://localhost:8080/cgi-bin/test.py → runs CGI script
______________________________________________________________________________________

# Checklist Before Finishing
□ Match requests to correct server/location.
□ Serve static files (GET) with proper Content-Type.
□ Handle directory listing (autoindex) when enabled.
□ Implement file upload (POST).
□ Implement DELETE.
□ Implement CGI (at least basic Python).
□ Return custom error pages (from config) for 404, 403, 500, etc.
□ Implement redirects (return directive).
□ Properly set Content-Length, Connection, and status codes.
□ Ensure keep-alive works for multiple requests.
______________________________________________________________________________________

# Final Note

<iframe src="https://giphy.com/embed/op4n9nZWhQiZ2" width="480" height="271" style="" frameBorder="0" class="giphy-embed" allowFullScreen></iframe><p><a href="https://giphy.com/gifs/shirokuma-cafe-op4n9nZWhQiZ2">via GIPHY</a></p>

