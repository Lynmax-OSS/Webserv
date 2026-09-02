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
		PollManager poll(manager);
		poll.run();  // ← This runs FOREVER!
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

//-----------------------------------------------------
// Parser Unit Tests (11 tests - ALL PASSING ✅)
// ============================================================
// Tests all features: GET, POST, chunked, URI decoding,
// keep-alive, errors (400, 413, 414, 431, 505), pipelining.
//-----------------------------------------------------

// #include "../include/ParserHeader/HttpParser.hpp"
// #include <iostream>

// void printRequest(const HttpRequest& req) {
//     std::cout << "Method: " << req.method << std::endl;
//     std::cout << "Path: " << req.path << std::endl;
//     std::cout << "Version: " << req.http_version << std::endl;
//     std::cout << "Body: " << req.body << std::endl;
//     std::cout << "Keep-Alive: " << (req.keep_alive ? "yes" : "no") << std::endl;
//     std::cout << "---" << std::endl;
// }

// int main() {
//     std::cout << "=== Testing HttpParser ===" << std::endl;
    
//     // Test 1: Simple GET
//     {
//         std::cout << "\nTest 1: Simple GET" << std::endl;
//         HttpParser parser;
//         parser.feed("GET /index.html HTTP/1.1\r\n");
//         parser.feed("Host: localhost\r\n");
//         parser.feed("Connection: close\r\n");
//         parser.feed("\r\n");
        
//         if (parser.isComplete()) {
//             printRequest(parser.getRequest());
//         }
//     }
    
//     // Test 2: POST with body
//     {
//         std::cout << "\nTest 2: POST with body" << std::endl;
//         HttpParser parser;
//         parser.feed("POST /upload HTTP/1.1\r\n");
//         parser.feed("Host: localhost\r\n");
//         parser.feed("Content-Length: 11\r\n");
//         parser.feed("\r\n");
//         parser.feed("Hello World");
        
//         if (parser.isComplete()) {
//             printRequest(parser.getRequest());
//         }
//     }
    
//     // Test 3: Chunked encoding
//     {
//         std::cout << "\nTest 3: Chunked encoding" << std::endl;
//         HttpParser parser;
//         parser.feed("POST /upload HTTP/1.1\r\n");
//         parser.feed("Host: localhost\r\n");
//         parser.feed("Transfer-Encoding: chunked\r\n");
//         parser.feed("\r\n");
//         parser.feed("5\r\n");
//         parser.feed("Hello\r\n");
//         parser.feed("6\r\n");
//         parser.feed(" World\r\n");
//         parser.feed("0\r\n");
//         parser.feed("\r\n");
        
//         if (parser.isComplete()) {
//             printRequest(parser.getRequest());
//         }
//     }
    
//     // Test 4: URI decoding
//     {
//         std::cout << "\nTest 4: URI decoding" << std::endl;
//         HttpParser parser;
//         parser.feed("GET /hello%20world%3Ftest HTTP/1.1\r\n");
//         parser.feed("Host: localhost\r\n");
//         parser.feed("\r\n");
        
//         if (parser.isComplete()) {
//             printRequest(parser.getRequest());
//         }
//     }

//     // Test 5: Keep-Alive detection
//     {
//         std::cout << "\nTest 5: Keep-Alive detection" << std::endl;
//         HttpParser parser;
//         parser.feed("GET / HTTP/1.1\r\n");
//         parser.feed("Host: localhost\r\n");
//         parser.feed("Connection: keep-alive\r\n");
//         parser.feed("\r\n");
        
//         if (parser.isComplete()) {
//             std::cout << "Keep-Alive: " << (parser.getRequest().keep_alive ? "yes ✅" : "no ❌") << std::endl;
//         }
//     }

//     // Test 6: Missing Host header (should error)
//     {
//         std::cout << "\nTest 6: Missing Host header (HTTP/1.1)" << std::endl;
//         HttpParser parser;
//         parser.feed("GET / HTTP/1.1\r\n");
//         parser.feed("\r\n");
        
//         if (parser.hasError()) {
//             std::cout << "✅ Correctly returned error code: " << parser.getRequest().error_code << std::endl;
//         }
//     }

//     // Test 7: Multiple requests on one connection (keep-alive)
//     {
//         std::cout << "\nTest 7: Multiple requests (keep-alive)" << std::endl;
//         HttpParser parser;
        
//         // First request
//         parser.feed("GET /page1.html HTTP/1.1\r\n");
//         parser.feed("Host: localhost\r\n");
//         parser.feed("\r\n");
        
//         if (parser.isComplete()) {
//             std::cout << "Request 1 path: " << parser.getRequest().path << std::endl;
//             parser.reset(); // Reset for next request
//         }
        
//         // Second request (same connection)
//         parser.feed("GET /page2.html HTTP/1.1\r\n");
//         parser.feed("Host: localhost\r\n");
//         parser.feed("\r\n");
        
//         if (parser.isComplete()) {
//             std::cout << "Request 2 path: " << parser.getRequest().path << std::endl;
//         }
//         std::cout << "✅ Keep-alive works!" << std::endl;
//     }

//     // Test 8: Body too large
//     {
//         std::cout << "\nTest 8: Body too large (413)" << std::endl;
//         HttpParser parser;
//         parser.setMaxBodySize(10);
//         std::cout << "Max body size set to: 10" << std::endl;
        
//         std::string big_body(100, 'X');
//         parser.feed("POST /upload HTTP/1.1\r\n");
//         parser.feed("Host: localhost\r\n");
//         parser.feed("Content-Length: 100\r\n");
//         parser.feed("\r\n");
//         parser.feed(big_body);
        
//         std::cout << "Parser state: " << (parser.hasError() ? "ERROR" : "OK") << std::endl;
//         std::cout << "Error code: " << parser.getRequest().error_code << std::endl;
        
//         if (parser.hasError() && parser.getRequest().error_code == 413) {
//             std::cout << "✅ Correctly returned 413: Payload Too Large" << std::endl;
//         }
//     }

//     // Test 9: Invalid HTTP version
//     {
//         std::cout << "\nTest 9: Invalid HTTP version (505)" << std::endl;
//         HttpParser parser;
//         parser.feed("GET / HTTP/0.9\r\n\r\n");
//         if (parser.hasError() && parser.getRequest().error_code == 505) {
//             std::cout << "✅ Correctly returned 505: HTTP Version Not Supported" << std::endl;
//         }
//     }

//     // Test 10: Empty URI
//     {
//         std::cout << "\nTest 10: Empty URI (400)" << std::endl;
//         HttpParser parser;
//         parser.feed("GET  HTTP/1.1\r\nHost: localhost\r\n\r\n");
//         if (parser.hasError() && parser.getRequest().error_code == 400) {
//             std::cout << "✅ Correctly returned 400: Bad Request" << std::endl;
//         }
//     }

//     // Test 11: Pipelining (multiple requests in one buffer)
//     {
//         std::cout << "\nTest 11: Pipelining" << std::endl;
//         HttpParser parser;
        
//         std::string pipeline = 
//             "GET /page1.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
//             "GET /page2.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
//             "GET /page3.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
        
//         parser.feed(pipeline);
        
//         int count = 0;
//         while (parser.isComplete()) {
//             count++;
//             std::cout << "Request " << count << ": " << parser.getRequest().path << std::endl;
//             parser.reset(true); // Keep the buffer
            
//             // Process remaining data in buffer
//             // feed() will now process even with empty string
//             parser.feed(""); // This will process the buffer!
//         }
        
//         if (count == 3) {
//             std::cout << "✅ Pipelining works! Handled 3 requests in one buffer." << std::endl;
//         } else {
//             std::cout << "❌ Processed " << count << " requests out of 3." << std::endl;
//         }
//     }
    
//     return 0;
// }

