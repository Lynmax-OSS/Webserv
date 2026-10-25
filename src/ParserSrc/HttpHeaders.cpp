#include "../../include/ParserHeader/HttpParser.hpp"
#include <cstdlib>

//finished headers example:
// _request.headers = {
//     "Host":             "localhost",
//     "Connection":       "keep-alive",
//     "Content-Length":   "11",
//     "User-Agent":       "curl/7.81"
// }
void HttpParser::_finishHeaders() {
    if (_request.headers.find("Connection") != _request.headers.end()) {
        std::string conn = _request.headers["Connection"];
        for (size_t i = 0; i < conn.size(); i++)
            conn[i] = std::tolower(conn[i]);
        if (conn == "keep-alive") 
            _request.keep_alive = true;
        else if (conn == "close") 
            _request.keep_alive = false;
    } 
    else if (_request.http_version == "HTTP/1.1") 
        _request.keep_alive = true;

    if (_request.http_version == "HTTP/1.1") {
        if (_request.headers.find("Host") == _request.headers.end()) {
            _state = ERROR;
            _request.error_code = 400; //Bad Request
            return ;
        }
    }

    bool is_chunked = false;
    if (_request.headers.find("Transfer-Encoding") != _request.headers.end()) {
        if (_request.headers["Transfer-Encoding"] == "chunked")
            is_chunked = true;
    }
    if (is_chunked == true) { //content-length = -1
        _state = PARSING_CHUNKED_SIZE;
        _chunk_size = 0;
        _chunk_bytes_read = 0;
        _waiting_for_chunk_end = false;
        return ;
    }

    if (_request.headers.find("Content-Length") != _request.headers.end()) {
        _request.content_length = atoi(_request.headers["Content-Length"].c_str());
        if (_request.content_length == 0) { // if content len = 0, then no body
            _state = COMPLETE;
            return ;
        }
        _state = BODY;
        return ;
    }
    _state = COMPLETE; //no Host issue, no chunked, no content-length.. done!!
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t");
    size_t end = s.find_last_not_of(" \t");
    if (start == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

//"Host: localhost" -> headers["Host"] = "localhost"
bool HttpParser::_parseHeaderLine(const std::string& line) {
    if (line.size() > 8192) { //Check header line length
        _setError(431);
        return false;
    }
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
        _setError(400);
        return false;
    }
    std::string key = line.substr(0, colon_pos);
    std::string value = trim(line.substr(colon_pos + 1));

    if (key.find(' ') != std::string::npos) { //Check for valid header format (no spaces in key)
        _setError(400); //Bad Request
        return false;
    }
    _request.headers[key] = value; //Store the header
    return true;
}

//process headers
void HttpParser::_processHeaders() {
    size_t newline_pos = _buffer.find("\r\n");
    if (newline_pos == std::string::npos)
        return ;
    std::string line = _buffer.substr(0, newline_pos);
    _buffer.erase(0, newline_pos + 2);
    if (line.empty()) { //blank line - EOHeaders
        _finishHeaders();
        return ;
    }
    if (!_parseHeaderLine(line))
        return ;
}
