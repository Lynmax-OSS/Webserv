#include "../../include/ParserHeader/HttpParser.hpp"
#include <algorithm>

//process body(content length)
void HttpParser::_processBody() {
                    // hello=world - 11 bytes         // "" - 0
    int bytes_needed = _request.content_length - _request.body.size(); //total bytes the body still needs
    if (bytes_needed <= 0) { // ifnothing to read, COMPLETE and exit function
        _state = COMPLETE;
        return ;
    }
    int bytes_available = _buffer.size(); //bytes are sitting in the buffer right now
    int bytes_to_read = std::min(bytes_needed, bytes_available); // if slow computer, then we process abit by abit - ps** It's not about a slow computer — it's about network fragmentation (TCP).
    if (bytes_to_read == 0) //if nothin to read, wait for more data
        return ;
    if (_request.body.size() + bytes_to_read > _max_body_size) { //check if body is too large (1MB)
        _setError(413);
        return ;
    }
    _request.body += _buffer.substr(0, bytes_to_read); //read bytes and store into _request.body
    _buffer.erase(0, bytes_to_read); // as we add in _request.body above, we erase from _buffer
    if (_request.body.size() == (size_t)_request.content_length) //if size is equal, means all bytes are processed
        _state = COMPLETE; //body in full is read
}
