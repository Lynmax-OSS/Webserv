#include "../../include/ParserHeader/HttpParser.hpp"
#include <cstdlib>

//process chunked size
void HttpParser::_processChunkedSize() {
    size_t newline_pos = _buffer.find("\r\n"); // Look for the end of the line (\r\n)
    if (newline_pos == std::string::npos) { //if no have full line, wait
        return ;
    }
    std::string line = _buffer.substr(0, newline_pos); //extract line and remove frm buffer
    _buffer.erase(0, newline_pos + 2);

    // Parse the chunk size into hexnum
    // Example: "5" means 5 bytes of data
    // Example: "1a" means 26 bytes of data
    char* endptr;
    _chunk_size = strtol(line.c_str(), &endptr, 16);

    // if chunk size is 0, this is the last chunk
    if (_chunk_size == 0) {
        _state = COMPLETE;
        return ;
    }
    // or else must read the chunk data
    _state = PARSING_CHUNKED_DATA;
    _chunk_bytes_read = 0;
}

//process chunked data
void HttpParser::_processChunkedData() {
    // How many bytes do we still need for this chunk?
    int bytes_needed = _chunk_size - _chunk_bytes_read; //check if need more bytes for this chunk
    int bytes_available = _buffer.size(); //available bytes in buffer
    int bytes_to_read = (bytes_needed < bytes_available) ? bytes_needed : bytes_available; // How many bytes can we read this time?

    // If there's nothing to read, wait for more data
    if (bytes_to_read == 0) {
        return ;
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