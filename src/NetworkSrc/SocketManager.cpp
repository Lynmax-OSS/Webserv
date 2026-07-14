#include "../../include/NetworkHeader/SocketManager.hpp"
#include "../../include/NetworkHeader/ServerSocket.hpp"

const std::vector<ServerSocket*>& SocketManager::getSocket()
{
	return (_sockets);
}

SocketManager::SocketManager(const std::vector<ServerConfig> &config)
{
	for (size_t i = 0; i < config.size(); i++)
	{
		ServerSocket* s = new ServerSocket(config[i]);
		_sockets.push_back(s);
	}
}

SocketManager::~SocketManager()
{
	for (size_t i = 0; i < _sockets.size(); i++)
		delete _sockets[i];
}