#ifndef WEBSERV_HPP
# define WEBSERV_HPP
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

struct Socket {
    int fd;
    explicit Socket(int domain, int type, int protocol)
        : fd(socket(domain, type, protocol)) {}
    ~Socket() { if (fd != -1) close(fd); }

    // Disable copying — a socket fd should have one owner
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
};

#endif