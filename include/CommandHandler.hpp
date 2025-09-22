#pragma once

#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include <string>

class Server;
class Client;

class CommandHandler {
  public:
	CommandHandler(Server *server);
	~CommandHandler(void);

	void handleCommand(Client &client, std::string const &message);

  private:
	Server *_server;

	CommandHandler(void);
	CommandHandler(CommandHandler const &src);
	CommandHandler &operator=(CommandHandler const &rhs);
};

#endif
