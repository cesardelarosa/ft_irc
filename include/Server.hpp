#pragma once

#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include <map>
#include <poll.h>
#include <string>
#include <vector>

class Server {
  public:
	Server(int port, std::string password);
	~Server();

	void start();

  private:
	int                        _port;
	std::string                _password;
	int                        _server_fd;
	std::vector<struct pollfd> _fds;
	std::map<int, Client *>    _clients;

	void _setupServerSocket();
	void _runEventLoop();
	void _handleNewConnection();
	void _handleClientData(size_t client_idx);
	void _removeClient(size_t client_idx);

	Server();
	Server(Server const &src);
	Server &operator=(Server const &rhs);
};

#endif
