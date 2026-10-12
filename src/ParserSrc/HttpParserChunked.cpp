#include "../../include/ParserHeader/HttpParser.hpp"

//process body(content length)
void HttpParser::_processBody() {
    // How many bytes do we still need?
    int bytes_needed = _request.content_length - _request.body.size();
    if (bytes_needed <= 0) { // if no need any more, then settle
        _state = COMPLETE;
        return ;
    }
    int bytes_available = _buffer.size(); //how many bytes available in buffer?
    int bytes_to_read = (bytes_needed < bytes_available) ? bytes_needed : bytes_available; //How many bytes can we read this time?
    if (bytes_to_read == 0) { //if nothin to read, wait for more data
        return ;
    }
    if (_request.body.size() + bytes_to_read > _max_body_size) { //check if body is too large
        _setError(413);
        return ;
    }
    _request.body += _buffer.substr(0, bytes_to_read); //read bytes
    _buffer.erase(0, bytes_to_read);
    if (_request.body.size() == (size_t)_request.content_length) { // check if have complete body
        _state = COMPLETE;
    }
}