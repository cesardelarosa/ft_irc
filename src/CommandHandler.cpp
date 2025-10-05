#include "CommandHandler.hpp"
#include "Replies.hpp"
#include "Server.hpp"
#include <iostream>
#include <sstream>

/**
 * @brief Constructs a new CommandHandler object and registers available
 * commands.
 * @param server A pointer to the main Server object, used to interact with the
 * server state.
 */
CommandHandler::CommandHandler(Server *server) : _server(server) {
	this->_commands["NICK"] = &CommandHandler::_handleNick;
	this->_commands["USER"] = &CommandHandler::_handleUser;
	this->_commands["PASS"] = &CommandHandler::_handlePass;
}

/**
 * @brief Destroys the CommandHandler object.
 */
CommandHandler::~CommandHandler() {
}

/**
 * @brief The main entry point for handling a command. It calls the parser and
 * executor.
 * @param client The client who sent the command.
 * @param message The raw command line received from the client.
 */
void CommandHandler::handleCommand(Client &client, std::string const &message) {
	_parseAndExecute(client, message);
}

/**
 * @brief Parses a raw command string, identifies the command and its arguments,
 * and dispatches it.
 * @details This function correctly handles IRC-style trailing arguments (those
 * prefixed with a ' :').
 * @param client The client who sent the command.
 * @param raw_command The raw command line to parse.
 */
void CommandHandler::_parseAndExecute(Client            &client,
                                      const std::string &raw_command) {
	std::string              line = raw_command;
	std::vector<std::string> args;

	// 1. Find trailing argument (prefixed with ' :')
	size_t colon_pos = line.find(" :");
	if (colon_pos != std::string::npos) {
		args.push_back(line.substr(colon_pos + 2));
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
		return;
	}

	// 3. Separate command and arguments
	std::string command = tokens[0];
	if (tokens.size() > 1) {
		args.insert(args.begin(), tokens.begin() + 1, tokens.end());
	}

	// 4. Dispatch the command
	std::map<std::string, CommandFunction>::iterator it =
	    this->_commands.find(command);

	if (it != this->_commands.end()) {
		(this->*(it->second))(client, args);
	} else {
		_server->sendReply(client, ERR_UNKNOWNCOMMAND(command));
	}
}

/**
 * @brief Handles the NICK command to set or change a user's nickname.
 * @param client The client sending the command.
 * @param args A vector of arguments. For NICK, it should contain the new
 * nickname.
 */
void CommandHandler::_handleNick(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (args.empty()) {
		_server->sendReply(client, ERR_NONICKNAMEGIVEN);
		return;
	}

	// NOTA: añadir la lógica para comprobar si el nick ya existe.
	client.setNickname(args[0]);
	std::cout << "Executing NICK command with arg: " << args[0] << std::endl;
}

/**
 * @brief Handles the USER command to register a user's details.
 * @param client The client sending the command.
 * @param args A vector of arguments containing username, hostname, servername,
 * and realname.
 */
void CommandHandler::_handleUser(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (args.size() < 4) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("USER"));
		return;
	}
	// NOTA: comprobar si el usuario ya está registrado.
	client.setUsername(args[0]);
	std::cout << "Executing USER command for user: " << args[0] << std::endl;
}

/**
 * @brief Handles the PASS command to authenticate a client with the server
 * password.
 * @param client The client sending the command.
 * @param args A vector of arguments containing the password.
 */
void CommandHandler::_handlePass(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (client.isAuthenticated()) {
		_server->sendReply(client, ERR_ALREADYREGISTRED);
		return;
	}

	if (args.empty()) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("PASS"));
		return;
	}

	// NOTA: comparar args[0] con la contraseña real del servidor.
	// Esta es una implementación simplificada.
	std::cout << "Executing PASS command." << std::endl;
	client.setAuthenticated(true);
}