#include "../../include/ParserHeader/HttpParser.hpp"

//constructor and reset
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

//main function - feed data into the parser
void HttpParser::feed(const std::string& data) {
    
    if (!data.empty()) { // Add new data to buffer
        _buffer += data;
    }
    // If we're in error or complete state, skip processing
    if (_state == COMPLETE || _state == ERROR) {
        return ;
    }
    // Keep processing until we run out of data or finish
    while (_buffer.size() > 0 && _state != COMPLETE && _state != ERROR) {
        size_t buffer_before = _buffer.size();
        State state_before = _state;
        if      (_state == REQUEST_LINE)         { _processRequestLine(); } 
        else if (_state == HEADERS)              { _processHeaders();     } 
        else if (_state == BODY)                 { _processBody();        } 
        else if (_state == PARSING_CHUNKED_SIZE) { _processChunkedSize(); } 
        else if (_state == PARSING_CHUNKED_DATA) { _processChunkedData(); }
        //if no byte consumed and no state change, stop
        if (_buffer.size() == buffer_before && _state == state_before)
            break ;
    }
}

void HttpParser::_setError(int error_code) {
    _state = ERROR;
    _request.error_code = error_code;
}

bool HttpParser::isComplete() const {
    return _state == COMPLETE;
}

bool HttpParser::hasError() const {
    return _state == ERROR;
}

const HttpRequest& HttpParser::getRequest() const {
    return _request;
}
