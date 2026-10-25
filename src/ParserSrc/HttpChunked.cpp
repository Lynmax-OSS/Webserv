#include "../../include/ParserHeader/HttpParser.hpp"
#include <cstdlib>

// exp: "5\r\nhello\r\n3\r\nabc\r\n0\r\n\r\n"
void HttpParser::_processChunkedSize() {
    size_t newline_pos = _buffer.find("\r\n");
    if (newline_pos == std::string::npos)
        return ;
    std::string line = _buffer.substr(0, newline_pos);
    _buffer.erase(0, newline_pos + 2);

    // Parse the chunk size into hexnum
    _chunk_size = strtol(line.c_str(), NULL, 16);

    // if chunk size is 0, this is the last chunk
    if (_chunk_size == 0) {
        _state = COMPLETE;
        return ;
    }
    // otherwise read the chunk data next
    _state = PARSING_CHUNKED_DATA;
    _chunk_bytes_read = 0;
    _waiting_for_chunk_end = false;
}

void HttpParser::_processChunkedData() {
    // If chunk data is already fully read, we're just waiting for \r\n.
    if (_waiting_for_chunk_end) {
        if (_buffer.size() >= 2 && _buffer[0] == '\r' && _buffer[1] == '\n') {
            _buffer.erase(0, 2);
            _state = PARSING_CHUNKED_SIZE;
            _waiting_for_chunk_end = false;
        }
        return ;
    }
    // How many bytes do we still need for this chunk?
    int bytes_needed = _chunk_size - _chunk_bytes_read;
    int bytes_available = _buffer.size();
    int bytes_to_read = bytes_available;
    if (bytes_needed < bytes_available)
        bytes_to_read = bytes_needed;

    if (bytes_to_read == 0)
        return ;
    if (_request.body.size() + bytes_to_read > _max_body_size) {
        _setError(413);
        return ;
    }
    // Read the bytes into the body
    _request.body += _buffer.substr(0, bytes_to_read);
    _buffer.erase(0, bytes_to_read);
    _chunk_bytes_read += bytes_to_read;
    // If the whole chunk has been read, wait for the trailing \r\n next time
    if (_chunk_bytes_read == _chunk_size)
        _waiting_for_chunk_end = true;
}
