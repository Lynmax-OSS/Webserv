#include "../../include/ParserHeader/HttpParser.hpp"
#include <sstream>
#include <cstdlib>

// GET /hello?name=jo HTTP/1.1\r\n
// Host: localhost\r\n
// \r\n

void HttpParser::_processRequestLine() {
    
    size_t newline_pos = _buffer.find("\r\n"); //Look for the end of the line (\r\n)
    if (newline_pos == std::string::npos) {
        return ;
    }
    std::string line = _buffer.substr(0, newline_pos); //GET /hello?name=jo HTTP/1.1\r\n
    _buffer.erase(0, newline_pos + 2); //+2.. remove \r\n

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

    if (!(iss >> method >> uri >> version)) { return false; }
//           GET     /hello?name=jo     HTTP/1.1\r\n
    if (method != "GET" && method != "POST" && method != "DELETE") { return false; }
    if (uri.empty() || uri[0] != '/') { _setError(400); return false; }
    if (version != "HTTP/1.1" && version != "HTTP/1.0") { _setError(505); return false; }
    if (uri.size() > 8192) { _setError(414); return false; } //Check URI length - common server limit, nginx uses 8k by default

    for (size_t i = 0; i < uri.size(); ++i) {
        char c = uri[i];
        if (!isalnum(c) && c != '/' && c != '.' && c != '-' && c != '_' &&
            c != '?' && c != '&' && c != '=' && c != '%' && c != '+') {
            _setError(400);
            return false;
        }
    }

    _request.method = method; //GET
    _request.uri = uri; //hello?name=jo
    _request.http_version = version; //HTTP/1.1\r\n
    _request.path = _decodeURI(uri); //if(hello?name=jo) if loop overwrites; if(/index.html) return true

    size_t qmark = uri.find('?');
    if (qmark != std::string::npos) {
        _request.query_string   = uri.substr(qmark + 1); //name=jo
        _request.path           = _decodeURI(uri.substr(0, qmark));//hello
    }
    return true;
}

//decode uri
std::string HttpParser::_decodeURI(const std::string& uri) {
    std::string result;

    for (size_t i = 0; i < uri.size(); ++i) {
        if (uri[i] == '%' && i + 2 < uri.size() && isxdigit(uri[i + 1]) && isxdigit(uri[i + 2])) {
            char hex[3] = {uri[i + 1], uri[i + 2], '\0'};
            long value = strtol(hex, NULL, 16);
            result += static_cast<char>(value);
            i += 2; //Skip the two hex digits
        } else if (uri[i] == '+') //+ in URL means space
            result += ' ';
        else
            result += uri[i];
    }
    return result;
}
