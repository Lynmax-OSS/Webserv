#ifndef HTTPPARSER_HPP
#define HTTPPARSER_HPP

#include <string>
#include <ctime>
#include "HttpRequest.hpp"

class HttpParser {
public:
    // Constructor
    HttpParser();
    
    // Main function - feed raw data into the parser
    void feed(const std::string& data);
    
    // Check status
    bool isComplete() const;
    bool hasError() const;
    
    // Get the parsed request
    const HttpRequest& getRequest() const;
    
    // Reset for the next request (keep-alive)
    void reset();
    void reset(bool keep_buffer); // NEW: For pipelining

    void setMaxBodySize(size_t size);
    time_t getStartTime() const;

    // For debugging only
    const std::string& getBuffer() const { return _buffer; }

    

private:
    // States the parser can be in
    enum State {
        REQUEST_LINE,           // Reading "GET /index.html HTTP/1.1"
        HEADERS,                // Reading headers like "Host: localhost"
        BODY,                   // Reading body (Content-Length)
        PARSING_CHUNKED_SIZE,   // Reading chunk size (hex number)
        PARSING_CHUNKED_DATA,   // Reading chunk data
        COMPLETE,               // Done! Request is fully parsed
        ERROR                   // Something went wrong
    };
    
    // State variables
    State _state;
    std::string _buffer;        // Leftover data from partial reads
    HttpRequest _request;       // The request being built
    std::string _current_line;  // Currently building a line
    
    // Chunked encoding variables
    size_t _chunk_size;
    size_t _chunk_bytes_read;
    size_t _max_body_size;

    time_t _start_time;
    
    // Processing functions (one per state)
    void _processRequestLine();
    void _processHeaders();
    void _processBody();
    void _processChunkedSize();
    void _processChunkedData();
    
    // Helper functions
    void _finishHeaders();
    bool _parseRequestLine(const std::string& line);
    bool _parseHeaderLine(const std::string& line);
    std::string _decodeURI(const std::string& uri);
    void _setError(int error_code);
};

#endif