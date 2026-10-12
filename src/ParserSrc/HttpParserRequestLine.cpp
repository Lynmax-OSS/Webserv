#include "../../include/ParserHeader/HttpParser.hpp"
#include <sstream>
#include <cstdlib>

//process request line
void HttpParser::_processRequestLine() {
    //Look for the end of the line (\r\n)
    size_t newline_pos = _buffer.find("\r\n");

    //If we don't have a full line yet, wait for more data
    if (newline_pos == std::string::npos) {
        return ;
    }

    //Extract the line and remove it from buffer
    std::string line = _buffer.substr(0, newline_pos);
    _buffer.erase(0, newline_pos + 2); //+2 for \r\n

    //Try to parse it
    if (!_parseRequestLine(line)) {
        if (_request.error_code == 0) {
            _setError(400); //Bad Request
        }
        return ;
    }
    _state = HEADERS; //move to next state
}

//parse request line
bool HttpParser::_parseRequestLine(const std::string& line) {
    //Example: "GET /index.html HTTP/1.1"
    std::istringstream iss(line);
    std::string method, uri, version;

    if (!(iss >> method >> uri >> version)) { //try ato extract the 3 parts
        return false;
    }
    if (method != "GET" && method != "POST" && method != "DELETE") { //check if method is valid
        return false;
    }
    _request.method = method;
    _request.uri = uri;
    _request.http_version = version;
    _request.path = _decodeURI(uri); //Decode the URI
    if (uri.size() > 8192) { //Check URI length
        _setError(414);
        return false;
    }
    for (size_t i = 0; i < uri.size(); ++i) { //Check for invalid characters
        char c = uri[i];
        if (!isalnum(c) && c != '/' && c != '.' && c != '-' && c != '_' &&
            c != '?' && c != '&' && c != '=' && c != '%' && c != '+') {
            _setError(400); //Bad Request
            return false;
        }
    }
    size_t qmark = uri.find('?'); //Extract query string if got
    if (qmark != std::string::npos) {
        _request.query_string = uri.substr(qmark + 1);
        _request.path = _decodeURI(uri.substr(0, qmark)); //path is everything before the ?
    }
    if (version != "HTTP/1.1" && version != "HTTP/1.0") { //Check for HTTP version
        _setError(505); //not supported
        return false;
    }
    if (uri.empty() || uri[0] != '/') { //Check for empty URI
        _setError(400); //Bad Request
        return false;
    }
    return true;
}

//decode uri
std::string HttpParser::_decodeURI(const std::string& uri) {
    std::string result;

    for (size_t i = 0; i < uri.size(); ++i) {
        if (uri[i] == '%' && i + 2 < uri.size()) {
            //Convert %XX(ascii) to character
            //Example: %20 -> space
            char hex[3] = {uri[i + 1], uri[i + 2], '\0'};
            char* endptr;
            long value = strtol(hex, &endptr, 16);
            result += static_cast<char>(value);
            i += 2; //Skip the two hex digits
        } else if (uri[i] == '+') { //+ in URL means space
            result += ' ';
        } else {
            result += uri[i];
        }
    }

    return result;
}