#include "../../include/ParserHeader/ClientConnection.hpp"
#include <ctime>

ClientConnection::ClientConnection() 
    : _fd(-1), _keep_alive(true) {
    updateActivity();
}

ClientConnection::ClientConnection(int fd) 
    : _fd(fd), _keep_alive(true) {
	updateActivity(); // Set initial time
}

void ClientConnection::updateActivity() {
    _last_activity = time(NULL);
}

bool ClientConnection::isTimeout(time_t current_time, int timeout_seconds) const {
    return (current_time - _last_activity) > timeout_seconds;
}

ClientConnection::~ClientConnection() {}

int ClientConnection::getFd() const {
    return _fd;
}

HttpParser& ClientConnection::getParser() {
    return _parser;
}

std::string& ClientConnection::getWriteBuffer() {
    return _write_buffer;
}

bool ClientConnection::isKeepAlive() const {
    return _keep_alive;
}

void ClientConnection::setKeepAlive(bool keep_alive) {
    _keep_alive = keep_alive;
}