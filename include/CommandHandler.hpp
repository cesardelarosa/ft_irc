#pragma once

#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "Client.hpp"
#include <map>
#include <string>
#include <vector>

class Server;
class Client;
class Channel;

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
	void _tryRegister(Client &client);

	// Registration commands
	void _handlePass(Client &client, const std::vector<std::string> &args);
	void _handleNick(Client &client, const std::vector<std::string> &args);
	void _handleUser(Client &client, const std::vector<std::string> &args);

	// Channel commands
	void _handleJoin(Client &client, const std::vector<std::string> &args);
	void _handlePart(Client &client, const std::vector<std::string> &args);
	void _handleInvite(Client &client, const std::vector<std::string> &args);
	void _handleTopic(Client &client, const std::vector<std::string> &args);
	void _handleMode(Client &client, const std::vector<std::string> &args);
	void _handleKick(Client &client, const std::vector<std::string> &args);
	void _handleList(Client &client, const std::vector<std::string> &args);

	// Messaging and User queries
	void _handlePrivmsg(Client &client, const std::vector<std::string> &args);
	void _handleNotice(Client &client, const std::vector<std::string> &args);
	void _handleWhois(Client &client, const std::vector<std::string> &args);

	// Connection and Capabilities
	void _handleQuit(Client &client, const std::vector<std::string> &args);
	void _handlePing(Client &client, const std::vector<std::string> &args);
	void _handlePong(Client &client, const std::vector<std::string> &args);
	void _handleCap(Client &client, const std::vector<std::string> &args);

	void _applyModes(Client &client, Channel &channel, const std::string &modes,
	                 const std::vector<std::string> &params, size_t paramIndex);

	CommandHandler();
	CommandHandler(CommandHandler const &src);
	CommandHandler &operator=(CommandHandler const &rhs);
};

#endif
