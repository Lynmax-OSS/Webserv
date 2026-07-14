#ifndef SOCKETMANAGER_HPP
# define SOCKETMANGER_HPP
# include "../ConfigHeader/ConfigParser.hpp"

class SocketManager
{
	public: 
		SocketManager(const std::vector<ServerConfig> &configs);
		~SocketManager();
		const std::vector<ServerSocket*>& getSocket() const;
	private:
		std::vector<ServerSocket*> _sockets;

		SocketManager(const SocketManager &other);
		SocketManager &operator=(const SocketManager &other);
};

#endif