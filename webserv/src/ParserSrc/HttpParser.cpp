#include "../../include/ParserHeader/HttpParser.hpp"
#include <sstream>
#include <cstdlib>
#include <iostream>

// ============================================
// CONSTRUCTOR & RESET
// ============================================

HttpParser::HttpParser() {
    reset();
    _max_body_size = 1024 * 1024; // 1MB default
    _start_time = time(NULL);
}

void HttpParser::reset() {
    reset(false); // Default: clear buffer
}


void HttpParser::setMaxBodySize(size_t size) {
    _max_body_size = size;
}

void HttpParser::reset(bool keep_buffer) {
    _state = REQUEST_LINE;
    if (!keep_buffer) {
        _buffer.clear();
    }
    _current_line.clear();
    _request = HttpRequest();
    _chunk_size = 0;
    _chunk_bytes_read = 0;
    _start_time = time(NULL);
}

time_t HttpParser::getStartTime() const {
    return _start_time;
}

// ============================================
// MAIN FUNCTION - Feed data into the parser
// ============================================

void HttpParser::feed(const std::string& data) {
    // Add new data to buffer
    if (!data.empty()) {
        _buffer += data;
    }

    // If we're in error or complete state, skip processing
    if (_state == COMPLETE || _state == ERROR) {
        return;
    }
    
    // Keep processing until we run out of data or finish
    while (_buffer.size() > 0 && _state != COMPLETE && _state != ERROR) {
        if (_state == REQUEST_LINE) {
            _processRequestLine();
        } else if (_state == HEADERS) {
            _processHeaders();
        } else if (_state == BODY) {
            _processBody();
        } else if (_state == PARSING_CHUNKED_SIZE) {
            _processChunkedSize();
        } else if (_state == PARSING_CHUNKED_DATA) {
            _processChunkedData();
        }
    }
}

// ============================================
// PROCESS REQUEST LINE
// ============================================

void HttpParser::_processRequestLine() {
    // Look for the end of the line (\r\n)
    size_t newline_pos = _buffer.find("\r\n");
    
    // If we don't have a full line yet, wait for more data
    if (newline_pos == std::string::npos) {
        return;
    }
    
    // Extract the line and remove it from buffer
    std::string line = _buffer.substr(0, newline_pos);
    _buffer.erase(0, newline_pos + 2); // +2 for \r\n
    
    // Try to parse it
    if (!_parseRequestLine(line)) {
        if (_request.error_code == 0) {
            _setError(400); // Bad Request
        }
        return;
    }
    
    // Move to next state
    _state = HEADERS;
}

// ============================================
// PROCESS HEADERS
// ============================================

void HttpParser::_processHeaders() {
    // Look for the end of the line (\r\n)
    size_t newline_pos = _buffer.find("\r\n");
    
    // If we don't have a full line yet, wait for more data
    if (newline_pos == std::string::npos) {
        return;
    }
    
    // Extract the line and remove it from buffer
    std::string line = _buffer.substr(0, newline_pos);
    _buffer.erase(0, newline_pos + 2);
    
    // If the line is empty, headers are done
    if (line.empty()) {
        _finishHeaders();
        return;
    }
    
    // Parse the header
    _parseHeaderLine(line);
}

// ============================================
// FINISH HEADERS - Decide what to do next
// ============================================

void HttpParser::_finishHeaders() {
    // Check for keep-alive
    if (_request.headers.find("Connection") != _request.headers.end()) {
        if (_request.headers["Connection"] == "keep-alive") {
            _request.keep_alive = true;
        } else if (_request.headers["Connection"] == "close") {
            _request.keep_alive = false;
        }
    } else if (_request.http_version == "HTTP/1.1") {
        // HTTP/1.1 defaults to keep-alive
        _request.keep_alive = true;
    }
    
    // HTTP/1.1 REQUIRES Host header
    if (_request.http_version == "HTTP/1.1") {
        if (_request.headers.find("Host") == _request.headers.end()) {
            _state = ERROR;
            _request.error_code = 400; // Bad Request
            return;
        }
    }

    // Check if we have chunked encoding
    bool is_chunked = false;
    if (_request.headers.find("Transfer-Encoding") != _request.headers.end()) {
        if (_request.headers["Transfer-Encoding"] == "chunked") {
            is_chunked = true;
        }
    }
    
    if (is_chunked) {
        // We need to parse chunked data
        _state = PARSING_CHUNKED_SIZE;
        _chunk_size = 0;
        _chunk_bytes_read = 0;
        return;
    }
    
    // Check if we have a Content-Length header
    if (_request.headers.find("Content-Length") != _request.headers.end()) {
        _request.content_length = atoi(_request.headers["Content-Length"].c_str());
        
        // If content-length is 0, there's no body
        if (_request.content_length == 0) {
            _state = COMPLETE;
            return;
        }
        
        // Otherwise, we need to read the body
        _state = BODY;
        return;
    }

    // No body at all - we're done!
    _state = COMPLETE;
}

// ============================================
// PROCESS BODY (Content-Length)
// ============================================

void HttpParser::_processBody() {
    // How many bytes do we still need?
    int bytes_needed = _request.content_length - _request.body.size();
    
    // If we don't need any more, we're done
    if (bytes_needed <= 0) {
        _state = COMPLETE;
        return;
    }
    
    // How many bytes are available in the buffer?
    int bytes_available = _buffer.size();
    
    // How many bytes can we read this time?
    int bytes_to_read = (bytes_needed < bytes_available) ? bytes_needed : bytes_available;
    
    // If there's nothing to read, wait for more data
    if (bytes_to_read == 0) {
        return;
    }

    // Check if body is too large
    if (_request.body.size() + bytes_to_read > _max_body_size) {
        _setError(413); // Payload Too Large
        return;
    }
    
    // Read the bytes
    _request.body += _buffer.substr(0, bytes_to_read);
    _buffer.erase(0, bytes_to_read);
    
    // Check if we have the complete body
    if (_request.body.size() == (size_t)_request.content_length) {
        _state = COMPLETE;
    }
}

// ============================================
// PROCESS CHUNKED SIZE
// ============================================

void HttpParser::_processChunkedSize() {
    // Look for the end of the line (\r\n)
    size_t newline_pos = _buffer.find("\r\n");
    
    // If we don't have a full line yet, wait for more data
    if (newline_pos == std::string::npos) {
        return;
    }
    
    // Extract the line and remove it from buffer
    std::string line = _buffer.substr(0, newline_pos);
    _buffer.erase(0, newline_pos + 2);
    
    // Parse the chunk size (hex number)
    // Example: "5" means 5 bytes of data
    // Example: "1a" means 26 bytes of data
    char* endptr;
    _chunk_size = strtol(line.c_str(), &endptr, 16);
    
    // If chunk size is 0, this is the last chunk
    if (_chunk_size == 0) {
        _state = COMPLETE;
        return;
    }
    
    // Otherwise, we need to read the chunk data
    _state = PARSING_CHUNKED_DATA;
    _chunk_bytes_read = 0;
}

// ============================================
// PROCESS CHUNKED DATA
// ============================================

void HttpParser::_processChunkedData() {
    // How many bytes do we still need for this chunk?
    int bytes_needed = _chunk_size - _chunk_bytes_read;
    
    // How many bytes are available in the buffer?
    int bytes_available = _buffer.size();
    
    // How many bytes can we read this time?
    int bytes_to_read = (bytes_needed < bytes_available) ? bytes_needed : bytes_available;
    
    // If there's nothing to read, wait for more data
    if (bytes_to_read == 0) {
        return;
    }
    
    // Read the bytes
    _request.body += _buffer.substr(0, bytes_to_read);
    _buffer.erase(0, bytes_to_read);
    _chunk_bytes_read += bytes_to_read;
    
    // If we've read the whole chunk
    if (_chunk_bytes_read == _chunk_size) {
        // We need to skip the trailing \r\n after the chunk data
        // But we might not have it yet, so we'll check in the next iteration
        _state = PARSING_CHUNKED_SIZE;
        
        // Check if the trailing \r\n is already in the buffer
        if (_buffer.size() >= 2 && _buffer[0] == '\r' && _buffer[1] == '\n') {
            _buffer.erase(0, 2);
        }
    }
}

// ============================================
// PARSE REQUEST LINE
// ============================================

bool HttpParser::_parseRequestLine(const std::string& line) {
    // Example: "GET /index.html HTTP/1.1"
    std::istringstream iss(line);
    std::string method, uri, version;
    
    // Try to extract the 3 parts
    if (!(iss >> method >> uri >> version)) {
        return false;
    }
    
    // Check if method is valid
    if (method != "GET" && method != "POST" && method != "DELETE") {
        return false;
    }
    
    // Store the values
    _request.method = method;
    _request.uri = uri;
    _request.http_version = version;
    
    // Decode the URI
    _request.path = _decodeURI(uri);
    
    // Check URI length
    if (uri.size() > 8192) {
        _setError(414); // URI Too Long
        return false;
    }
    
    // Check for invalid characters in URI
    for (size_t i = 0; i < uri.size(); ++i) {
        // Only allow certain characters
        char c = uri[i];
        if (!isalnum(c) && c != '/' && c != '.' && c != '-' && c != '_' && 
            c != '?' && c != '&' && c != '=' && c != '%' && c != '+') {
            _setError(400); // Bad Request
            return false;
        }
    }

    // Extract query string if there is one
    size_t qmark = uri.find('?');
    if (qmark != std::string::npos) {
        _request.query_string = uri.substr(qmark + 1);
        // Path is everything before the ?
        _request.path = _decodeURI(uri.substr(0, qmark));
    }

    // Check for HTTP version
    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        _setError(505); // HTTP Version Not Supported
        return false;
    }

    // Check for empty URI
    if (uri.empty() || uri[0] != '/') {
        _setError(400); // Bad Request
        return false;
    }
    
    return true;
}

// ============================================
// PARSE HEADER LINE
// ============================================

bool HttpParser::_parseHeaderLine(const std::string& line) {
    // Example: "Host: localhost"
    size_t colon_pos = line.find(':');
    
    // If there's no colon, it's not a valid header
    if (colon_pos == std::string::npos) {
        return false;
    }
    
    // Split into key and value
    std::string key = line.substr(0, colon_pos);
    std::string value = line.substr(colon_pos + 1);
    
    // Remove leading/trailing spaces from value
    // Example: " localhost " -> "localhost"
    size_t start = value.find_first_not_of(" \t");
    if (start != std::string::npos) {
        value = value.substr(start);
    }
    size_t end = value.find_last_not_of(" \t");
    if (end != std::string::npos) {
        value = value.substr(0, end + 1);
    }
    
    // Check header line length
    if (line.size() > 8192) {
        _setError(431); // Request Header Fields Too Large
        return false;
    }

    // Check for valid header format (no spaces in key)
    if (key.find(' ') != std::string::npos) {
        _setError(400); // Bad Request
        return false;
    }

    // Store the header
    _request.headers[key] = value;
    
    return true;
}

// ============================================
// DECODE URI
// ============================================

std::string HttpParser::_decodeURI(const std::string& uri) {
    std::string result;
    
    for (size_t i = 0; i < uri.size(); ++i) {
        if (uri[i] == '%' && i + 2 < uri.size()) {
            // Convert %XX to character
            // Example: %20 -> space
            char hex[3] = {uri[i + 1], uri[i + 2], '\0'};
            char* endptr;
            long value = strtol(hex, &endptr, 16);
            result += static_cast<char>(value);
            i += 2; // Skip the two hex digits
        } else if (uri[i] == '+') {
            // + in URL means space
            result += ' ';
        } else {
            result += uri[i];
        }
    }
    
    return result;
}

// ============================================
// HELPER FUNCTION TO SET ERROR STATE
// ============================================
void HttpParser::_setError(int error_code) {
    _state = ERROR;
    _request.error_code = error_code;
}

// ============================================
// GETTERS
// ============================================

bool HttpParser::isComplete() const {
    return _state == COMPLETE;
}

bool HttpParser::hasError() const {
    return _state == ERROR;
}

const HttpRequest& HttpParser::getRequest() const {
    return _request;
}