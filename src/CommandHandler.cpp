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
	std::string              line = raw_command;
	std::vector<std::string> args;

	// 1. Find trailing argument (prefixed with ' :')
	size_t colon_pos = line.find(" :");
	if (colon_pos != std::string::npos) {
		// The trailing part is everything after the " :"
		args.push_back(line.substr(colon_pos + 2));
		// The rest of the line is processed for command and normal args
		line.erase(colon_pos);
	}

	// 2. Tokenize the rest of the line (command and other args)
	std::stringstream        ss(line);
	std::string              token;
	std::vector<std::string> tokens;
	while (ss >> token) {
		tokens.push_back(token);
	}

	if (tokens.empty()) {
		return; // Empty or whitespace-only command line
	}

	// 3. Separate command and arguments
	std::string command = tokens[0];
	// Insert the non-trailing args at the beginning of the args vector
	if (tokens.size() > 1) {
		args.insert(args.begin(), tokens.begin() + 1, tokens.end());
	}

	// 4. Dispatch the command
	std::map<std::string, CommandFunction>::iterator it =
	    this->_commands.find(command);

	if (it != this->_commands.end()) {
		(this->*(it->second))(client, args);
	} else {
		std::cout << "Unknown command: " << command << std::endl;
		// Here you would typically send a numeric reply like ERR_UNKNOWNCOMMAND
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
