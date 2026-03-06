#include "CommandHandler.hpp"
#include "Channel.hpp"
#include "Replies.hpp"
#include "Server.hpp"
#include <iostream>
#include <sstream>
#include <sys/socket.h>

/**
 * @brief Constructs a new CommandHandler and registers all available commands.
 * @param server Pointer to the main Server object.
 */
CommandHandler::CommandHandler(Server *server) : _server(server) {
	this->_commands["PASS"] = &CommandHandler::_handlePass;
	this->_commands["NICK"] = &CommandHandler::_handleNick;
	this->_commands["USER"] = &CommandHandler::_handleUser;
	this->_commands["JOIN"] = &CommandHandler::_handleJoin;
	this->_commands["PART"] = &CommandHandler::_handlePart;
	this->_commands["PRIVMSG"] = &CommandHandler::_handlePrivmsg;
	this->_commands["QUIT"] = &CommandHandler::_handleQuit;
}

CommandHandler::~CommandHandler() {
}

/**
 * @brief Main entry point for handling a command from a client.
 * @param client The client who sent the command.
 * @param message The raw command line.
 */
void CommandHandler::handleCommand(Client &client, std::string const &message) {
	_parseAndExecute(client, message);
}

/**
 * @brief Parses a raw command string and dispatches it to the appropriate
 * handler.
 * @details Handles IRC trailing arguments (prefixed with ' :') correctly.
 * Blocks non-registration commands if the client is not yet registered.
 * @param client The client who sent the command.
 * @param raw_command The raw command line to parse.
 */
void CommandHandler::_parseAndExecute(Client            &client,
                                      const std::string &raw_command) {
	std::string line = raw_command;
	std::string trailing;
	bool        has_trailing = false;

	// 1. Extract trailing argument (prefixed with ' :')
	size_t colon_pos = line.find(" :");
	if (colon_pos != std::string::npos) {
		trailing = line.substr(colon_pos + 2);
		has_trailing = true;
		line.erase(colon_pos);
	}

	// 2. Tokenize the rest of the line
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
	std::string              command = tokens[0];
	std::vector<std::string> args;
	if (tokens.size() > 1) {
		args.insert(args.begin(), tokens.begin() + 1, tokens.end());
	}
	// Trailing argument goes last
	if (has_trailing) {
		args.push_back(trailing);
	}

	// 4. Block commands from unregistered clients (except PASS, NICK, USER)
	if (!client.isRegistered() && command != "PASS" && command != "NICK" &&
	    command != "USER" && command != "QUIT") {
		_server->sendReply(client, ERR_NOTREGISTERED);
		return;
	}

	// 5. Dispatch the command
	std::map<std::string, CommandFunction>::iterator it =
	    this->_commands.find(command);

	if (it != this->_commands.end()) {
		(this->*(it->second))(client, args);
	} else {
		if (client.isRegistered()) {
			_server->sendReply(client, ERR_UNKNOWNCOMMAND(command));
		}
	}
}

// ═══════════════════════════════════════════════════════════════
//  Registration helpers
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Checks if the client has completed PASS + NICK + USER and, if so,
 * marks them as registered and sends RPL_WELCOME.
 */
void CommandHandler::_tryRegister(Client &client) {
	if (client.isRegistered())
		return;
	if (!client.hasPass() || !client.hasNick() || !client.hasUser())
		return;

	client.setRegistered(true);
	_server->sendReply(client, RPL_WELCOME(client.getNickname(),
	                                       client.getUsername(), "localhost"));
	std::cout << "Client " << client.getFd() << " registered as "
	          << client.getNickname() << std::endl;
}

// ═══════════════════════════════════════════════════════════════
//  PASS
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handlePass(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (client.isRegistered()) {
		_server->sendReply(client, ERR_ALREADYREGISTRED);
		return;
	}
	if (args.empty()) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("PASS"));
		return;
	}
	if (args[0] != _server->getPassword()) {
		_server->sendReply(client, ERR_PASSWDMISMATCH);
		return;
	}

	client.setHasPass(true);
	std::cout << "Client " << client.getFd() << " passed authentication."
	          << std::endl;
	_tryRegister(client);
}

// ═══════════════════════════════════════════════════════════════
//  NICK
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleNick(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (args.empty()) {
		std::string current = client.getNickname().empty()
		                          ? std::string("*")
		                          : client.getNickname();
		_server->sendReply(client, ERR_NONICKNAMEGIVEN(current));
		return;
	}

	std::string nick = args[0];

	// Validate nickname: must start with a letter or special char, no spaces
	if (nick.empty() || nick.find(' ') != std::string::npos) {
		std::string current = client.getNickname().empty()
		                          ? std::string("*")
		                          : client.getNickname();
		_server->sendReply(client, ERR_ERRONEUSNICKNAME(current, nick));
		return;
	}

	// Check for nickname collisions
	Client *existing = _server->getClientByNickname(nick);
	if (existing != NULL && existing != &client) {
		std::string current = client.getNickname().empty()
		                          ? std::string("*")
		                          : client.getNickname();
		_server->sendReply(client, ERR_NICKNAMEINUSE(current, nick));
		return;
	}

	// If already registered, broadcast the nick change
	if (client.isRegistered()) {
		std::string old_prefix = client.getPrefix();
		client.setNickname(nick);
		// Notify the client itself
		std::string msg = ":" + old_prefix + " NICK " + nick + "\r\n";
		send(client.getFd(), msg.c_str(), msg.length(), 0);
	} else {
		client.setNickname(nick);
		_tryRegister(client);
	}

	std::cout << "Client " << client.getFd() << " nick set to: " << nick
	          << std::endl;
}

// ═══════════════════════════════════════════════════════════════
//  USER
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleUser(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (client.isRegistered()) {
		_server->sendReply(client, ERR_ALREADYREGISTRED);
		return;
	}
	if (args.size() < 4) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("USER"));
		return;
	}

	client.setUsername(args[0]);
	// args[3] is the realname (trailing argument)
	client.setRealname(args[3]);

	std::cout << "Client " << client.getFd() << " user set to: " << args[0]
	          << std::endl;
	_tryRegister(client);
}

// ═══════════════════════════════════════════════════════════════
//  JOIN
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleJoin(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (args.empty()) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("JOIN"));
		return;
	}

	// Support joining multiple channels separated by commas
	std::stringstream ss(args[0]);
	std::string       channel_name;
	std::string       key = (args.size() > 1) ? args[1] : "";

	while (std::getline(ss, channel_name, ',')) {
		if (channel_name.empty())
			continue;

		// Channel names must start with '#'
		if (channel_name[0] != '#' && channel_name[0] != '&') {
			_server->sendReply(
			    client, ERR_NOSUCHCHANNEL(client.getNickname(), channel_name));
			continue;
		}

		Channel *channel = _server->getChannel(channel_name);
		bool     is_new = (channel == NULL);

		if (is_new) {
			channel = _server->createChannel(channel_name);
		} else {
			// Already a member? Skip silently
			if (channel->isMember(&client))
				continue;

			// Check invite-only
			if (channel->isInviteOnly() &&
			    !channel->isInvited(client.getNickname())) {
				_server->sendReply(
				    client,
				    ERR_INVITEONLYCHAN(client.getNickname(), channel_name));
				continue;
			}

			// Check channel key
			if (channel->hasKey() && key != channel->getKey()) {
				_server->sendReply(
				    client,
				    ERR_BADCHANNELKEY(client.getNickname(), channel_name));
				continue;
			}

			// Check user limit
			if (channel->hasUserLimit() &&
			    channel->getMemberCount() >= channel->getUserLimit()) {
				_server->sendReply(
				    client,
				    ERR_CHANNELISFULL(client.getNickname(), channel_name));
				continue;
			}
		}

		// Add the client to the channel
		channel->addMember(&client);
		if (is_new) {
			channel->addOperator(&client);
		}

		// Remove from invite list if they were invited
		channel->removeInvited(client.getNickname());

		// Broadcast JOIN to all channel members (including the joining client)
		channel->broadcastMessage(
		    ":" + client.getPrefix() + " JOIN " + channel_name, NULL);

		// Send topic
		if (!channel->getTopic().empty()) {
			_server->sendReply(client,
			                   RPL_TOPIC(client.getNickname(), channel_name,
			                             channel->getTopic()));
		}

		// Send NAMES list
		_server->sendReply(client,
		                   RPL_NAMREPLY(client.getNickname(), channel_name,
		                                channel->getMemberListString()));
		_server->sendReply(client,
		                   RPL_ENDOFNAMES(client.getNickname(), channel_name));
	}
}

// ═══════════════════════════════════════════════════════════════
//  PART
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handlePart(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (args.empty()) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("PART"));
		return;
	}

	std::string reason = (args.size() > 1) ? args[1] : "";

	std::stringstream ss(args[0]);
	std::string       channel_name;

	while (std::getline(ss, channel_name, ',')) {
		if (channel_name.empty())
			continue;

		Channel *channel = _server->getChannel(channel_name);
		if (channel == NULL) {
			_server->sendReply(
			    client, ERR_NOSUCHCHANNEL(client.getNickname(), channel_name));
			continue;
		}

		if (!channel->isMember(&client)) {
			_server->sendReply(
			    client, ERR_NOTONCHANNEL(client.getNickname(), channel_name));
			continue;
		}

		// Build PART message
		std::string part_msg =
		    ":" + client.getPrefix() + " PART " + channel_name;
		if (!reason.empty()) {
			part_msg += " :" + reason;
		}

		// Broadcast PART to all members (including the leaving client)
		channel->broadcastMessage(part_msg, NULL);

		// Remove the client from the channel
		channel->removeMember(&client);

		// Delete channel if empty
		if (channel->isEmpty()) {
			_server->removeChannel(channel_name);
		}
	}
}

// ═══════════════════════════════════════════════════════════════
//  PRIVMSG
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handlePrivmsg(Client                         &client,
                                    const std::vector<std::string> &args) {
	if (args.empty()) {
		_server->sendReply(client, ERR_NORECIPIENT(client.getNickname()));
		return;
	}
	if (args.size() < 2) {
		_server->sendReply(client, ERR_NOTEXTTOSEND(client.getNickname()));
		return;
	}

	std::string target = args[0];
	std::string text = args[1];

	// Target is a channel
	if (target[0] == '#' || target[0] == '&') {
		Channel *channel = _server->getChannel(target);
		if (channel == NULL) {
			_server->sendReply(client,
			                   ERR_NOSUCHCHANNEL(client.getNickname(), target));
			return;
		}
		if (!channel->isMember(&client)) {
			_server->sendReply(
			    client, ERR_CANNOTSENDTOCHAN(client.getNickname(), target));
			return;
		}

		// Broadcast to all channel members except sender
		channel->broadcastMessage(":" + client.getPrefix() + " PRIVMSG " +
		                              target + " :" + text,
		                          &client);
	}
	// Target is a user
	else {
		Client *target_client = _server->getClientByNickname(target);
		if (target_client == NULL) {
			_server->sendReply(client,
			                   ERR_NOSUCHNICK(client.getNickname(), target));
			return;
		}

		std::string msg = ":" + client.getPrefix() + " PRIVMSG " + target +
		                  " :" + text + "\r\n";
		send(target_client->getFd(), msg.c_str(), msg.length(), 0);
	}
}

// ═══════════════════════════════════════════════════════════════
//  QUIT
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleQuit(Client                         &client,
                                 const std::vector<std::string> &args) {
	std::string reason = args.empty() ? "Client quit" : args[0];

	std::cout << "Client " << client.getFd() << " quit: " << reason
	          << std::endl;

	// The actual cleanup (removing from channels, closing socket) is handled
	// by Server::_removeClient, which is called when recv() returns 0 after
	// the client closes the connection. We just need to notify channels.
	_server->removeClientFromAllChannels(&client);

	// Send an ERROR message to the client before disconnecting
	std::string error_msg = "ERROR :Closing Link: " + client.getNickname() +
	                        " (Quit: " + reason + ")\r\n";
	send(client.getFd(), error_msg.c_str(), error_msg.length(), 0);
}
