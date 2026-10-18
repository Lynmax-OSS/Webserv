#include "../../include/ParserHeader/HttpParser.hpp"
#include <cstdlib>

//process headers
void HttpParser::_processHeaders() {
    size_t newline_pos = _buffer.find("\r\n"); //Look for the end of the line (\r\n)
    if (newline_pos == std::string::npos) { //if we dont have the full line yet, wait for more data
        return ;
    }
    std::string line = _buffer.substr(0, newline_pos); // extract line and remove it frm buffer
    _buffer.erase(0, newline_pos + 2);
    if (line.empty()) { // if line is empty, headers are done
        _finishHeaders();
        return ;
    }
    _parseHeaderLine(line);
}

//parse header line
bool HttpParser::_parseHeaderLine(const std::string& line) {
    size_t colon_pos = line.find(':'); //Example: "Host: localhost"
    if (colon_pos == std::string::npos) {
        return false; //If there's no colon, it's not a valid header
    }

    std::string key = line.substr(0, colon_pos);
    std::string value = line.substr(colon_pos + 1);

    size_t start = value.find_first_not_of(" \t"); //Remove front and behind spaces from value
    if (start != std::string::npos) {
        value = value.substr(start);
    }
    size_t end = value.find_last_not_of(" \t");
    if (end != std::string::npos) {
        value = value.substr(0, end + 1);
    }
    if (line.size() > 8192) { //Check header line length
        _setError(431);
        return false;
    }
    if (key.find(' ') != std::string::npos) { //Check for valid header format (no spaces in key)
        _setError(400); //Bad Request
        return false;
    }
    _request.headers[key] = value; //Store the header
    return true;
}

//Finish headers - decide on what to do next
void HttpParser::_finishHeaders() {
    if (_request.headers.find("Connection") != _request.headers.end()) { //Check for keep-alive
        if (_request.headers["Connection"] == "keep-alive") {
            _request.keep_alive = true;
        } else if (_request.headers["Connection"] == "close") {
            _request.keep_alive = false;
        }
    } else if (_request.http_version == "HTTP/1.1") {
        _request.keep_alive = true; //HTTP/1.1 defaults to keep-alive
    }
    if (_request.http_version == "HTTP/1.1") { //HTTP/1.1 REQUIRES Host header
        if (_request.headers.find("Host") == _request.headers.end()) {
            _state = ERROR;
            _request.error_code = 400; //Bad Request
            return ;
        }
    }
    //Check if we have chunked encoding
    bool is_chunked = false;
    if (_request.headers.find("Transfer-Encoding") != _request.headers.end()) {
        if (_request.headers["Transfer-Encoding"] == "chunked") {
            is_chunked = true;
        }
    }
    if (is_chunked) {
        _state = PARSING_CHUNKED_SIZE;
        _chunk_size = 0;
        _chunk_bytes_read = 0;
        return ;
    }
    //Check if content-length exist
    if (_request.headers.find("Content-Length") != _request.headers.end()) {
        _request.has_content_length = true;
        _request.content_length = atoi(_request.headers["Content-Length"].c_str());
        if (_request.has_content_length && _request.content_length > _max_body_size) {
            _state = ERROR;
            _request.error_code = 413;
            return ;
        }
        if (_request.content_length == 0) { // if content len = 0, then no body
            _state = COMPLETE;
            return ;
        }
        _state = BODY;  //Otherwise, we need to read the body
        return ;
    }
    _state = COMPLETE; //no body at all.. done!!
}