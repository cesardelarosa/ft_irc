#pragma once

#ifndef SERVER_HPP
#define SERVER_HPP

#include "Channel.hpp"
#include "Client.hpp"
#include "CommandHandler.hpp"
#include "EventManager.hpp"
#include "Socket.hpp"
#include <csignal>
#include <map>
#include <string>
#include <vector>

extern volatile sig_atomic_t g_shutdown;

class Server {
  public:
	Server(int port, std::string password);
	~Server();

	void start();
	void sendReply(Client &client, const std::string &message);
	void sendToClient(Client &client, const std::string &message);

	// Accessors for CommandHandler to use
	const std::string       &getPassword() const;
	std::map<int, Client *> &getClients();
	Client                  *getClientByNickname(const std::string &nick);
	Channel                 *getChannel(const std::string &name);
	Channel                 *createChannel(const std::string &name);
	void                     removeChannel(const std::string &name);
	void                     removeClientFromAllChannels(Client *client);

  private:
	int                              _port;
	std::string                      _password;
	std::map<int, Client *>          _clients;
	std::map<std::string, Channel *> _channels;
	Socket                           _serverSocket;
	EventManager                     _eventManager;
	CommandHandler                   _commandHandler;

	void _setupServerSocket();
	void _runEventLoop();
	void _handleNewConnection();
	bool _handleClientData(int client_idx);
	void _handleClientWrite(int client_idx);
	void _updatePollEvents();
	void _removeClient(int client_idx);
	void _processClientCommands(Client &client);

	Server();
	Server(Server const &src);
	Server &operator=(Server const &rhs);
};

#endif
