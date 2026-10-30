#ifndef CLIENTCONNECTION_HPP
#define CLIENTCONNECTION_HPP

#include "HttpParser.hpp"
#include <string>

class ClientConnection {
public:
    // Constructor
    ClientConnection(); 
    ClientConnection(int fd);
    ~ClientConnection();
    
    // Getter
    int getFd() const;
    HttpParser& getParser();
    std::string& getWriteBuffer();
    
    // Keep-alive management
    bool isKeepAlive() const;
    void setKeepAlive(bool keep_alive);

	void updateActivity();
    bool isTimeout(time_t current_time, int timeout_seconds) const;

    bool isReadClosed() const;
    void setReadClosed(bool v);
    
private:
    int _fd;
    HttpParser _parser;
    std::string _write_buffer;
    bool _keep_alive;
	bool _read_closed;
	time_t _last_activity;
};

#endif
