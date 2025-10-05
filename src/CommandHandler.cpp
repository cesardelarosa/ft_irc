#include "CommandHandler.hpp"
#include "Server.hpp"
#include <iostream>
#include <sstream>

CommandHandler::CommandHandler(Server *server) : _server(server) {
	this->_commands["NICK"] = &CommandHandler::_handleNick;
	this->_commands["USER"] = &CommandHandler::_handleUser;
	this->_commands["PASS"] = &CommandHandler::_handlePass;
}

CommandHandler::~CommandHandler() {
}

void CommandHandler::handleCommand(Client &client, std::string const &message) {
	_parseAndExecute(client, message);
}

void CommandHandler::_parseAndExecute(Client            &client,
                                      const std::string &raw_command) {
	std::stringstream ss(raw_command);
	std::string       command;
	ss >> command;

	std::vector<std::string> args;
	std::string              arg;
	while (ss >> arg) {
		args.push_back(arg);
	}

	std::map<std::string, CommandFunction>::iterator it =
	    this->_commands.find(command);

	if (it != this->_commands.end()) {
		(this->*(it->second))(client, args);
	} else {
		std::cout << "Unknown command: " << command << std::endl;
		// Enviar una respuesta de error numérico al cliente
	}
}

void CommandHandler::_handleNick(Client                         &client,
                                 const std::vector<std::string> &args) {
	(void)client;
	if (args.empty()) {
		std::cout << "NICK command received with no arguments." << std::endl;
		return;
	}
	std::cout << "Executing NICK command with arg: " << args[0] << std::endl;
}

void CommandHandler::_handleUser(Client                         &client,
                                 const std::vector<std::string> &args) {
	(void)client;
	if (args.size() < 4) {
		std::cout << "USER command received with insufficient arguments."
		          << std::endl;
		return;
	}
	std::cout << "Executing USER command for user: " << args[0] << std::endl;
}

void CommandHandler::_handlePass(Client                         &client,
                                 const std::vector<std::string> &args) {
	(void)client;
	if (args.empty()) {
		std::cout << "PASS command received with no arguments." << std::endl;
		return;
	}
	std::cout << "Executing PASS command." << std::endl;
}
