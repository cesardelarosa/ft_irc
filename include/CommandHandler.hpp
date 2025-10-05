#pragma once

#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "Client.hpp"
#include <map>
#include <string>
#include <vector>

class Server;
class Client;

class CommandHandler {
  public:
	CommandHandler(Server *server);
	~CommandHandler();

	void handleCommand(Client &client, std::string const &message);

  private:
	Server *_server;

	typedef void (CommandHandler::*CommandFunction)(
	    Client &client, const std::vector<std::string> &args);

	std::map<std::string, CommandFunction> _commands;

	void _parseAndExecute(Client &client, const std::string &raw_command);

	void _handleNick(Client &client, const std::vector<std::string> &args);
	void _handleUser(Client &client, const std::vector<std::string> &args);
	void _handlePass(Client &client, const std::vector<std::string> &args);

	CommandHandler();
	CommandHandler(CommandHandler const &src);
	CommandHandler &operator=(CommandHandler const &rhs);
};

#endif
