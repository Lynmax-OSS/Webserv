# Integration Guide - Member 2's HTTP Parser

## 📋 Overview

I've built a **complete HTTP/1.1 request parser** that converts raw HTTP requests into structured C++ objects. This document explains how Member 1 (Network/Sockets) and Member 3 (Response/Handling) should integrate with it.

---

## 📁 File Structure

webserv/
├── include/
│ └── ParserHeader/
│ ├── HttpRequest.hpp # Request data structure
│ ├── HttpParser.hpp # Parser class
│ └── ClientConnection.hpp # Per-client connection state
├── src/
│ └── ParserSrc/
│ ├── HttpParser.cpp # Parser implementation
│ └── ClientConnection.cpp # Connection management
└── Makefile # Must include parser files


---

## 🔧 What the Parser Does

| Feature   								 Status |
|---------------------------------------------------|
| Parses request line (GET, POST, DELETE) 	 	 ✅ |
| Parses HTTP headers | 						 ✅ |
| Handles body with Content-Length  			 ✅ |
| Handles chunked transfer encoding  			 ✅ |
| URI decoding (%20 → space, etc.)  			 ✅ |
| Keep-Alive detection | 						 ✅ |
| Host header validation (HTTP/1.1)  			 ✅ |
| Multiple requests on same connection  		 ✅ |
| Body size limits (413) | 						 ✅ |
| HTTP version validation (505)  				 ✅ |
| Pipelining (multiple requests in one buffer)	 ✅ |
| Timeout tracking  							 ✅ |

---

## 📊 The HttpRequest Struct

```cpp
struct HttpRequest {
    std::string method;          // "GET", "POST", "DELETE"
    std::string uri;             // Original URI "/index.html?name=john"
    std::string path;            // Decoded path "/index.html"
    std::string query_string;    // "name=john"
    std::string http_version;    // "HTTP/1.1"
    
    std::map<std::string, std::string> headers;  // All headers
    std::string body;            // Request body (decoded)
    
    bool keep_alive;             // true if Connection: keep-alive
    int content_length;          // -1 if not specified
    int error_code;              // 0 = OK, 400+ = error
};

## Error Codes

Code	Meaning
0		No error (request is valid)
400		Bad Request
413		Payload Too Large
414		URI Too Long
431		Request Header Fields Too Large
505		HTTP Version Not Supported

🔌 Member 1: Integration with PollManager

//-----------------------
Step 1: Add Includes
//-----------------------
PollManager.hpp:

#ifndef POLLMANAGER_HPP
#define POLLMANAGER_HPP

#include "../NetworkHeader/ServerSocket.hpp"
#include "../NetworkHeader/SocketManager.hpp"
#include "../ParserHeader/ClientConnection.hpp"  // ← ADD THIS
#include <poll.h>
#include <map>                                    // ← ADD THIS

class PollManager {
    // ...
};

//-----------------------
Step 2: Add Client Storage
//-----------------------
PollManager.hpp - Add member variable:

class PollManager {
    // ...
private:
    SocketManager &_manager;
    std::vector<pollfd> _poll_fds;
    std::map<int, ClientConnection> _clients;  // ← ADD THIS
    // ...
};

//-----------------------
Step 3: Create ClientConnection on New Connection
//-----------------------

PollManager.cpp - handleNewConnections():

void PollManager::handleNewConnections(int server_fd) {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        std::cerr << "accept() failed: " << strerror(errno) << "\n";
        return;
    }
    
    std::cout << "Client connected on fd " << client_fd << "\n";
    
    // ✅ CREATE ClientConnection
    _clients[client_fd] = ClientConnection(client_fd);
	//
    addFd(client_fd, POLLIN | POLLOUT);
}

//-----------------------
Step 4: Parse Requests in handleClientRead()
//-----------------------
PollManager.cpp - handleClientRead():

void PollManager::handleClientRead(int client_fd) {
    char buf[4096];
    ssize_t bytes = read(client_fd, buf, sizeof(buf) - 1);
    
    if (bytes <= 0) {
        std::cout << "Client fd " << client_fd << " disconnected\n";
        _clients.erase(client_fd);
        close(client_fd);
        removeFd(client_fd);
        return;
    }
    
    // ✅ Feed data to parser
    ClientConnection& conn = _clients[client_fd];
    conn.getParser().feed(std::string(buf, bytes));
    conn.updateActivity();  // Reset timeout timer
    
    // ✅ Check if request is complete
    if (conn.getParser().isComplete()) {
        const HttpRequest& req = conn.getParser().getRequest();
        
        // Check for parsing errors
        if (req.error_code != 0) {
            // Send error response (or let Member 3 handle)
            std::string error = "HTTP/1.1 " + std::to_string(req.error_code) + " Error\r\n\r\n";
            write(client_fd, error.c_str(), error.size());
        } else {
            // ✅ Valid request - pass to Member 3
            std::cout << "✅ Received: " << req.method << " " << req.path << std::endl;
            
            // Member 3 will handle the request and fill conn.getWriteBuffer()
            // handleRequest(req, conn.getWriteBuffer());
            
            // Send response from write buffer
            if (!conn.getWriteBuffer().empty()) {
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

//-----------------------
Step 5: Add Timeout Checks
//-----------------------
PollManager.cpp - run():

void PollManager::run() {
    std::cout << "Server running...\n";
    
    while (true) {
        // ✅ Use timeout for timeout checks (1000ms = 1 second)
        int ready = poll(_poll_fds.data(), _poll_fds.size(), 1000);
        
        // ✅ Check for timeouts on clients
        time_t now = time(NULL);
        for (std::map<int, ClientConnection>::iterator it = _clients.begin();
             it != _clients.end();) {
            if (it->second.isTimeout(now, 60)) {  // 60 second timeout
                std::cout << "Timeout on fd " << it->first << std::endl;
                close(it->first);
                removeFd(it->first);
                _clients.erase(it++);
            } else {
                ++it;
            }
        }
        
        if (ready == -1)
            throw std::runtime_error("poll() failed");
        
        // ... rest of poll loop ...
    }
}

//-----------------------
Step 6: Update Makefile
//-----------------------
Add parser source files:

makefile
SRCS = src/main.cpp \
       src/ParserSrc/HttpParser.cpp \
       src/ParserSrc/ClientConnection.cpp \
       src/ConfigSrc/ConfigParser.cpp \
       src/ConfigSrc/ConfigValidator.cpp \
       src/ConfigSrc/Tokenizer.cpp \
       src/NetworkSrc/PollManager.cpp \
       src/NetworkSrc/ServerSocket.cpp \
       src/NetworkSrc/SocketManager.cpp


📤 Member 3: Request Handling
Member 3 receives the HttpRequest object and produces a response.

//-----------------------
How to Access the Request
//-----------------------

void handleRequest(const HttpRequest& req, std::string& response_buffer) {
    // 1. Check the method
    if (req.method == "GET") {
        // Serve static file: req.path
        // Example: req.path == "/index.html"
    } else if (req.method == "POST") {
        // Handle upload: req.body contains the data
        // req.headers["Content-Type"] tells you the format
    } else if (req.method == "DELETE") {
        // Delete file: req.path
    }
    
    // 2. Check for errors
    if (req.error_code != 0) {
        // Send error page
        response_buffer = buildErrorResponse(req.error_code);
        return;
    }
    
    // 3. Build response
    response_buffer = buildHttpResponse(status, headers, body);
}

//-----------------------
What You Get
//-----------------------

// Access request parts
req.method        // "GET", "POST", "DELETE"
req.path          // "/index.html" (decoded)
req.query_string  // "name=john"
req.headers       // map of all headers
req.body          // Request body (decoded)
req.keep_alive    // true/false
req.error_code    // 0 = OK

// Example: Check Content-Type
if (req.headers.find("Content-Type") != req.headers.end()) {
    std::string content_type = req.headers["Content-Type"];
}

//-----------------------//
//-----------------------//
🧪 Testing Your Integration
//-----------------------//

Simple Test
After integration, you should be able to:

bash
# Compile
make re

# Run server
./webserv configs/default.conf

# In another terminal:
curl -v http://localhost:8080/index.html
Expected Output
Server:

text
Server running...
Client connected on fd 4
✅ Received: GET /index.html
curl:

text
HTTP/1.1 200 OK
Content-Type: text/html

//-----------------------
📋 Integration Checklist
//-----------------------
Member 1 (Network/Sockets)

□ Add #include "ParserHeader/ClientConnection.hpp" to PollManager.hpp
□ Add #include <map> to PollManager.hpp
□ Add std::map<int, ClientConnection> _clients; to PollManager class
□ Add #include "ParserHeader/ClientConnection.hpp" to PollManager.cpp
□ Create ClientConnection in handleNewConnections()
□ Call conn.getParser().feed() in handleClientRead()
□ Call conn.updateActivity() in handleClientRead()
□ Check req.error_code for errors
□ Handle keep-alive properly
□ Add timeout checks in run()
□ Update Makefile to compile parser files

Member 3 (Request/Response)

□ Implement handleRequest() function
□ Use req.method to route requests
□ Use req.path to find files
□ Use req.body for POST data
□ Use req.headers for content type
□ Build proper HTTP responses
□ Set Connection: close or keep-alive
□ Handle error codes (400, 404, 413, 500, etc.)

✅ Status
Component			Status
Parser (Member 2)	✅ Complete
Config (Member 1)	✅ Complete
Network (Member 1)	⬜ Needs integration
Response (Member 3)	⬜ In progress
CGI (Member 3)		⬜ In progress

## ⚠️ Current Status

**Member 2's parser is 100% complete and tested.** 
**Member 1 needs to integrate the parser into PollManager.** 
**Member 3 needs to implement request handling.**

//The "Common Issues" section below is for reference during integration.//

🆘 Common Issues
Issue: "No matching constructor" error
Fix: Make sure ClientConnection has a default constructor:

ClientConnection();  // ✅ Required for map[]
Issue: Parser not detecting complete request
Fix: Make sure you're calling conn.getParser().reset() after processing each request.

Issue: Keep-alive not working
Fix: Don't close the socket if req.keep_alive is true.

Issue: Timeout not working
Fix: Call conn.updateActivity() every time you receive data.

Last updated: 		2026-08-04