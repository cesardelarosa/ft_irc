#pragma once

#ifndef SERVER_HPP
#define SERVER_HPP

#include "Channel.hpp"
#include "Client.hpp"
#include "CommandHandler.hpp"
#include <map>
#include <poll.h>
#include <string>
#include <vector>

class Server {
  public:
	Server(int port, std::string password);
	~Server();

	void start();
	void sendReply(const Client &client, const std::string &message);

  private:
	int                        _port;
	std::string                _password;
	int                        _server_fd;
	std::vector<struct pollfd> _fds;
	std::map<int, Client *>    _clients;
	CommandHandler             _commandHandler;

	void _setupServerSocket();
	void _runEventLoop();
	void _handleNewConnection();
	void _handleClientData(size_t client_idx);
	void _removeClient(size_t client_idx);
	void _processClientCommands(Client &client);

	Server();
	Server(Server const &src);
	Server &operator=(Server const &rhs);
};

#endif
