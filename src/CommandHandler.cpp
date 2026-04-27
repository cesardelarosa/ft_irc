#include "CommandHandler.hpp"
#include "Channel.hpp"
#include "Replies.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <sstream>

/**
 * @brief Constructs a new CommandHandler and registers all available commands.
 * @param server Pointer to the main Server object.
 */
CommandHandler::CommandHandler(Server *server) : _server(server), _commands() {
	this->_commands["PASS"] = &CommandHandler::_handlePass;
	this->_commands["NICK"] = &CommandHandler::_handleNick;
	this->_commands["USER"] = &CommandHandler::_handleUser;
	this->_commands["JOIN"] = &CommandHandler::_handleJoin;
	this->_commands["PART"] = &CommandHandler::_handlePart;
	this->_commands["PRIVMSG"] = &CommandHandler::_handlePrivmsg;
	this->_commands["QUIT"] = &CommandHandler::_handleQuit;
	this->_commands["INVITE"] = &CommandHandler::_handleInvite;
	this->_commands["TOPIC"] = &CommandHandler::_handleTopic;
	this->_commands["MODE"] = &CommandHandler::_handleMode;
	this->_commands["KICK"] = &CommandHandler::_handleKick;
	this->_commands["PING"] = &CommandHandler::_handlePing;
	this->_commands["PONG"] = &CommandHandler::_handlePong;
	this->_commands["NOTICE"] = &CommandHandler::_handleNotice;
	this->_commands["CAP"] = &CommandHandler::_handleCap;
	this->_commands["WHOIS"] = &CommandHandler::_handleWhois;
	this->_commands["LIST"] = &CommandHandler::_handleList;
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

	if (!line.empty() && line[0] == ':') {
		size_t prefix_end = line.find(' ');
		if (prefix_end != std::string::npos) {
			line.erase(0, prefix_end + 1);
		} else {
			return;
		}
	}

	while (!line.empty() && line[0] == ' ') {
		line.erase(0, 1);
	}
	if (line.empty())
		return;

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

	// 3. Separate command and arguments (commands are case-insensitive)
	std::string              command = toUpper(tokens[0]);
	std::vector<std::string> args;
	if (tokens.size() > 1) {
		args.insert(args.begin(), tokens.begin() + 1, tokens.end());
	}
	// Trailing argument goes last
	if (has_trailing) {
		args.push_back(trailing);
	}

	// 4. Block commands from unregistered clients
	if (!client.isRegistered() && command != "PASS" && command != "NICK" &&
	    command != "USER" && command != "QUIT" && command != "CAP" &&
	    command != "PING" && command != "PONG") {
		_server->sendReply(client, ERR_NOTREGISTERED(client.getNickname()));
		return;
	}

	// 5. Dispatch the command
	std::map<std::string, CommandFunction>::iterator it =
	    this->_commands.find(command);

	if (it != this->_commands.end()) {
		(this->*(it->second))(client, args);
	} else {
		if (client.isRegistered()) {
			_server->sendReply(
			    client, ERR_UNKNOWNCOMMAND(client.getNickname(), command));
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
	_server->sendReply(client,
	                   RPL_WELCOME(client.getNickname(), client.getUsername(),
	                               client.getHostname()));
	std::cout << LOG_SERVER << LOG_NICK << client.getNickname() << LOG_R
	          << " registered " << ANSI_DIM << "(fd " << client.getFd() << ")"
	          << LOG_R << std::endl;
}

// ═══════════════════════════════════════════════════════════════
//  PASS
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handlePass(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (client.isRegistered()) {
		_server->sendReply(client, ERR_ALREADYREGISTRED(client.getNickname()));
		return;
	}
	if (args.empty()) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("PASS"));
		return;
	}
	if (args[0] != _server->getPassword()) {
		_server->sendReply(client, ERR_PASSWDMISMATCH(client.getNickname()));
		return;
	}

	client.setHasPass(true);
	std::cout << LOG_SERVER << ANSI_DIM << "fd " << client.getFd()
	          << " passed auth" << LOG_R << std::endl;
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
	std::string current = client.getNickname().empty() ? std::string("*")
	                                                   : client.getNickname();

	// Validate nickname per RFC 2812:
	// - Must start with a letter or special char ([]\\`_^{|})
	// - Subsequent chars: letters, digits, special, or hyphen
	// - Max 9 characters
	// - Cannot contain @, !, *, #, &, space, :, or comma
	if (nick.empty() || nick.length() > 9 ||
	    (!std::isalpha(nick[0]) &&
	     std::string("[]\\`_^{|}").find(nick[0]) == std::string::npos)) {
		_server->sendReply(client, ERR_ERRONEUSNICKNAME(current, nick));
		return;
	}
	for (size_t j = 1; j < nick.size(); ++j) {
		char c = nick[j];
		if (!std::isalnum(c) && c != '-' &&
		    std::string("[]\\`_^{|}").find(c) == std::string::npos) {
			_server->sendReply(client, ERR_ERRONEUSNICKNAME(current, nick));
			return;
		}
	}

	if (_isBotNickname(nick)) {
		_server->sendReply(client, ERR_NICKNAMEINUSE(current, nick));
		return;
	}

	// Check for nickname collisions
	Client *existing = _server->getClientByNickname(nick);
	if (existing != NULL && existing != &client) {
		_server->sendReply(client, ERR_NICKNAMEINUSE(current, nick));
		return;
	}

	// If already registered, broadcast the nick change
	if (client.isRegistered()) {
		std::string old_prefix = client.getPrefix();
		client.setNickname(nick);
		// Notify the client itself
		std::string msg = ":" + old_prefix + " NICK " + nick;
		_server->sendToClient(client, msg + "\r\n");

		// Notify all common channels
		std::map<std::string, Channel *> &channels = _server->getChannels();
		for (std::map<std::string, Channel *>::iterator it = channels.begin();
		     it != channels.end(); ++it) {
			if (it->second->isMember(&client)) {
				it->second->broadcastMessage(msg, &client);
			}
		}
	} else {
		client.setNickname(nick);
		_tryRegister(client);
	}

	std::cout << LOG_SERVER << ANSI_DIM << "fd " << client.getFd() << LOG_R
	          << " nick -> " << LOG_NICK << nick << LOG_R << std::endl;
}

// ═══════════════════════════════════════════════════════════════
//  USER
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleUser(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (client.isRegistered()) {
		_server->sendReply(client, ERR_ALREADYREGISTRED(client.getNickname()));
		return;
	}
	if (args.size() < 4) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("USER"));
		return;
	}

	client.setUsername(args[0]);
	client.setRealname(args[3]);

	std::cout << LOG_SERVER << ANSI_DIM << "fd " << client.getFd() << LOG_R
	          << " user -> " << ANSI_BRIGHT_CYAN << args[0] << LOG_R
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

	std::vector<std::string> keys;
	if (args.size() > 1) {
		std::stringstream ss_keys(args[1]);
		std::string       tmp_key;
		while (std::getline(ss_keys, tmp_key, ',')) {
			keys.push_back(tmp_key);
		}
	}

	size_t idx = 0;
	while (std::getline(ss, channel_name, ',')) {
		if (channel_name.empty())
			continue;

		std::string key = (idx < keys.size()) ? keys[idx] : "";
		idx++;

		// Channel names must start with '#' or '&' and follow basic RFC rules
		if (channel_name.length() < 2 || channel_name.length() > 50 ||
		    (channel_name[0] != '#' && channel_name[0] != '&') ||
		    channel_name.find_first_of(" ,\x07\r\n") != std::string::npos) {
			_server->sendReply(
			    client, ERR_NOSUCHCHANNEL(client.getNickname(), channel_name));
			continue;
		}

		Channel *channel = _server->getChannel(channel_name);
		bool     is_new = (channel == NULL);

		if (is_new) {
			channel = _server->createChannel(channel_name);
			if (channel == NULL) {
				_server->sendReply(
				    client,
				    ERR_NOSUCHCHANNEL(client.getNickname(), channel_name));
				continue;
			}
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

	if (_isBotNickname(target)) {
		_handleBotPrivmsg(client.getNickname(), text);
		return;
	}

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
		_handleBotPrivmsg(target, text);
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
		_server->sendToClient(*target_client, msg);
	}
}

// ═══════════════════════════════════════════════════════════════
//  Internal Bot
// ═══════════════════════════════════════════════════════════════

bool CommandHandler::_isBotNickname(const std::string &nick) const {
	return toIrcLower(nick) == "bot";
}

void CommandHandler::_handleBotPrivmsg(const std::string &target,
                                       const std::string &text) {
	if (text.empty() || text[0] != '!')
		return;

	std::string reply = _getBotReply(text);
	if (reply.empty())
		return;

	_sendBotNotice(target, reply);
}

std::string CommandHandler::_getBotReply(const std::string &text) const {
	if (text == "!help") {
		return "commands: !help !ping !time";
	}
	if (text == "!ping") {
		return "pong";
	}
	if (text == "!time") {
		return "time " + _getBotTime();
	}
	return "unknown command";
}

std::string CommandHandler::_getBotTime() const {
	std::time_t now = std::time(NULL);
	std::tm    *local_time = std::localtime(&now);
	char        buffer[20];

	if (local_time == NULL ||
	    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
	                  local_time) == 0) {
		return "unavailable";
	}
	return std::string(buffer);
}

void CommandHandler::_sendBotNotice(const std::string &target,
                                    const std::string &text) {
	std::string msg = ":Bot!bot@ircserv NOTICE " + target + " :" + text;

	if (!target.empty() && (target[0] == '#' || target[0] == '&')) {
		Channel *channel = _server->getChannel(target);
		if (channel != NULL) {
			channel->broadcastMessage(msg, NULL);
		}
	} else {
		Client *target_client = _server->getClientByNickname(target);
		if (target_client != NULL) {
			_server->sendToClient(*target_client, msg + "\r\n");
		}
	}
}

// ═══════════════════════════════════════════════════════════════
//  QUIT
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleQuit(Client                         &client,
                                 const std::vector<std::string> &args) {
	std::string reason = args.empty() ? "Client quit" : args[0];

	std::cout << LOG_DISCONNECT << LOG_NICK << client.getNickname() << LOG_R
	          << " quit: " << ANSI_DIM << reason << LOG_R << std::endl;

	// Sent an ERROR message to the client before disconnecting
	std::string error_msg = "ERROR :Closing Link: " + client.getNickname() +
	                        " (Quit: " + reason + ")\r\n";
	_server->sendToClient(client, error_msg);

	client.setQuitReason(reason);
	client.setDisconnected();
}

// ═══════════════════════════════════════════════════════════════
//  INVITE
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleInvite(Client                         &client,
                                   const std::vector<std::string> &args) {
	if (args.size() < 2) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("INVITE"));
		return;
	}

	std::string targetNick = args[0];
	std::string channelName = args[1];

	Channel *channel = _server->getChannel(channelName);
	if (!channel) {
		_server->sendReply(
		    client, ERR_NOSUCHCHANNEL(client.getNickname(), channelName));
		return;
	}

	if (!channel->isMember(&client)) {
		_server->sendReply(client,
		                   ERR_NOTONCHANNEL(client.getNickname(), channelName));
		return;
	}

	if (channel->isInviteOnly() && !channel->isOperator(&client)) {
		_server->sendReply(
		    client, ERR_CHANOPRIVSNEEDED(client.getNickname(), channelName));
		return;
	}

	Client *target = _server->getClientByNickname(targetNick);
	if (!target) {
		_server->sendReply(client,
		                   ERR_NOSUCHNICK(client.getNickname(), targetNick));
		return;
	}

	if (channel->isMember(target)) {
		_server->sendReply(
		    client,
		    ERR_USERONCHANNEL(client.getNickname(), targetNick, channelName));
		return;
	}

	channel->addInvited(targetNick);

	_server->sendReply(
	    client, RPL_INVITING(client.getNickname(), targetNick, channelName));

	std::string msg = ":" + client.getPrefix() + " INVITE " + targetNick +
	                  " :" + channelName + "\r\n";

	_server->sendToClient(*target, msg);
}

// ═══════════════════════════════════════════════════════════════
//  TOPIC
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleTopic(Client                         &client,
                                  const std::vector<std::string> &args) {
	if (args.empty()) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("TOPIC"));
		return;
	}

	std::string channelName = args[0];
	Channel    *channel = _server->getChannel(channelName);

	if (!channel) {
		_server->sendReply(
		    client, ERR_NOSUCHCHANNEL(client.getNickname(), channelName));
		return;
	}

	if (args.size() == 1) {
		if (channel->getTopic().empty())
			_server->sendReply(client,
			                   RPL_NOTOPIC(client.getNickname(), channelName));
		else
			_server->sendReply(client,
			                   RPL_TOPIC(client.getNickname(), channelName,
			                             channel->getTopic()));
		return;
	}

	if (!channel->isMember(&client)) {
		_server->sendReply(client,
		                   ERR_NOTONCHANNEL(client.getNickname(), channelName));
		return;
	}

	if (channel->isTopicRestricted() && !channel->isOperator(&client)) {
		_server->sendReply(
		    client, ERR_CHANOPRIVSNEEDED(client.getNickname(), channelName));
		return;
	}

	std::string newTopic = args[1];
	channel->setTopic(newTopic);

	std::string msg =
	    ":" + client.getPrefix() + " TOPIC " + channelName + " :" + newTopic;

	channel->broadcastMessage(msg, NULL);
}

// ═══════════════════════════════════════════════════════════════
//  MODE
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleMode(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (args.empty()) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("MODE"));
		return;
	}

	std::string channelName = args[0];
	Channel    *channel = _server->getChannel(channelName);

	if (!channel) {
		_server->sendReply(
		    client, ERR_NOSUCHCHANNEL(client.getNickname(), channelName));
		return;
	}

	if (args.size() == 1) {
		std::string modes = "+";
		std::string modes_params = "";
		if (channel->isInviteOnly())
			modes += "i";
		if (channel->isTopicRestricted())
			modes += "t";
		if (channel->hasKey()) {
			modes += "k";
			modes_params += " " + channel->getKey();
		}
		if (channel->hasUserLimit()) {
			modes += "l";
			std::stringstream ss;
			ss << channel->getUserLimit();
			modes_params += " " + ss.str();
		}

		_server->sendReply(client, "324 " + client.getNickname() + " " +
		                               channelName + " " + modes +
		                               modes_params);
		return;
	}

	if (!channel->isOperator(&client)) {
		_server->sendReply(
		    client, ERR_CHANOPRIVSNEEDED(client.getNickname(), channelName));
		return;
	}

	std::string modes = args[1];

	std::vector<std::string> params;
	if (args.size() > 2)
		params.insert(params.begin(), args.begin() + 2, args.end());

	_applyModes(client, *channel, modes, params, 0);
}

void CommandHandler::_applyModes(Client &client, Channel &channel,
                                 const std::string              &modes,
                                 const std::vector<std::string> &params,
                                 size_t                          paramIndex) {
	bool        adding = true;
	std::string applied_modes;
	std::string applied_params;
	bool        pending_sign = false;
	char        current_sign = '+';

	for (size_t i = 0; i < modes.size(); i++) {
		char c = modes[i];
		if (c == '+') {
			adding = true;
			current_sign = '+';
			pending_sign = true;
			continue;
		}
		if (c == '-') {
			adding = false;
			current_sign = '-';
			pending_sign = true;
			continue;
		}

		bool mode_applied = false;

		if (c == 'i') {
			channel.setInviteOnly(adding);
			mode_applied = true;
		} else if (c == 't') {
			channel.setTopicRestricted(adding);
			mode_applied = true;
		} else if (c == 'k') {
			if (adding && paramIndex < params.size()) {
				channel.setKey(params[paramIndex]);
				applied_params += " " + params[paramIndex++];
				mode_applied = true;
			} else if (!adding) {
				channel.removeKey();
				if (paramIndex < params.size()) {
					applied_params += " " + params[paramIndex++];
				}
				mode_applied = true;
			}
		} else if (c == 'l') {
			if (adding && paramIndex < params.size()) {
				long limit = std::atol(params[paramIndex].c_str());
				if (limit > 0) {
					channel.setUserLimit(static_cast<size_t>(limit));
					applied_params += " " + params[paramIndex];
					mode_applied = true;
				}
				paramIndex++;
			} else if (!adding) {
				channel.removeUserLimit();
				mode_applied = true;
			}
		} else if (c == 'o') {
			if (paramIndex < params.size()) {
				Client *target =
				    _server->getClientByNickname(params[paramIndex]);
				if (target && channel.isMember(target)) {
					if (adding)
						channel.addOperator(target);
					else
						channel.removeOperator(target);
					applied_params += " " + params[paramIndex++];
					mode_applied = true;
				} else {
					paramIndex++;
				}
			}
		} else {
			_server->sendReply(
			    client, ERR_UNKNOWNMODE(client.getNickname(), std::string(1, c)));
		}

		if (mode_applied) {
			if (pending_sign || applied_modes.empty()) {
				applied_modes += current_sign;
				pending_sign = false;
			}
			applied_modes += c;
		}
	}

	if (!applied_modes.empty()) {
		std::string msg = ":" + client.getPrefix() + " MODE " +
		                  channel.getName() + " " + applied_modes +
		                  applied_params;
		channel.broadcastMessage(msg, NULL);
	}
}

// ═══════════════════════════════════════════════════════════════
//  KICK
// ═══════════════════════════════════════════════════════════════
void CommandHandler::_handleKick(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (args.size() < 2) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("KICK"));
		return;
	}

	std::string channelNamesStr = args[0];
	std::string targetNicksStr = args[1];
	std::string reason = (args.size() > 2) ? args[2] : client.getNickname();

	std::vector<std::string> channels;
	std::stringstream        ssChan(channelNamesStr);
	std::string              t;
	while (std::getline(ssChan, t, ',')) {
		if (!t.empty())
			channels.push_back(t);
	}

	std::vector<std::string> targets;
	std::stringstream        ssTarg(targetNicksStr);
	while (std::getline(ssTarg, t, ',')) {
		if (!t.empty())
			targets.push_back(t);
	}

	if (channels.empty() || targets.empty())
		return;

	bool multiple_channels = (channels.size() > 1);
	for (size_t j = 0; j < targets.size(); ++j) {
		std::string targetNick = targets[j];
		std::string channelName = multiple_channels && j < channels.size()
		                              ? channels[j]
		                              : channels[0];

		Channel *channel = _server->getChannel(channelName);
		if (!channel) {
			_server->sendReply(
			    client, ERR_NOSUCHCHANNEL(client.getNickname(), channelName));
			continue;
		}
		if (!channel->isOperator(&client)) {
			_server->sendReply(client, ERR_CHANOPRIVSNEEDED(
			                               client.getNickname(), channelName));
			continue;
		}

		Client *target = _server->getClientByNickname(targetNick);
		if (!target || !channel->isMember(target)) {
			_server->sendReply(client,
			                   ERR_USERNOTINCHANNEL(client.getNickname(),
			                                        targetNick, channelName));
			continue;
		}

		std::string msg = ":" + client.getPrefix() + " KICK " + channelName +
		                  " " + targetNick + " :" + reason;
		channel->broadcastMessage(msg, NULL);
		channel->removeMember(target);
		if (channel->isEmpty())
			_server->removeChannel(channelName);
	}
}

// ═══════════════════════════════════════════════════════════════
//  PING
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handlePing(Client                         &client,
                                 const std::vector<std::string> &args) {
	if (args.empty()) {
		_server->sendReply(client, ERR_NEEDMOREPARAMS("PING"));
		return;
	}
	std::string msg =
	    ":" + std::string("ircserv") + " PONG ircserv :" + args[0] + "\r\n";
	_server->sendToClient(client, msg);
}

// ═══════════════════════════════════════════════════════════════
//  PONG
// ═══════════════════════════════════════════════════════════════
void CommandHandler::_handlePong(Client                         &client,
                                 const std::vector<std::string> &args) {
	(void)client;
	(void)args;
	// PONG activity update happens on network recv automatically, no action
	// needed.
}

// ═══════════════════════════════════════════════════════════════
//  NOTICE
// ═══════════════════════════════════════════════════════════════
void CommandHandler::_handleNotice(Client                         &client,
                                   const std::vector<std::string> &args) {
	if (args.empty() || args.size() < 2)
		return;

	std::string target = args[0];
	std::string text = args[1];

	if (target[0] == '#' || target[0] == '&') {
		Channel *channel = _server->getChannel(target);
		if (channel && channel->isMember(&client)) {
			channel->broadcastMessage(":" + client.getPrefix() + " NOTICE " +
			                              target + " :" + text,
			                          &client);
		}
	} else {
		Client *target_client = _server->getClientByNickname(target);
		if (target_client) {
			std::string msg = ":" + client.getPrefix() + " NOTICE " + target +
			                  " :" + text + "\r\n";
			_server->sendToClient(*target_client, msg);
		}
	}
}

// ═══════════════════════════════════════════════════════════════
//  CAP
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleCap(Client                         &client,
                                const std::vector<std::string> &args) {
	if (!args.empty() && args[0] == "LS") {
		std::string msg = ":" + std::string("ircserv") + " CAP * LS :\r\n";
		_server->sendToClient(client, msg);
	}
}

// ═══════════════════════════════════════════════════════════════
//  WHOIS
// ═══════════════════════════════════════════════════════════════
void CommandHandler::_handleWhois(Client                         &client,
                                  const std::vector<std::string> &args) {
	if (args.empty()) {
		_server->sendReply(client, ERR_NONICKNAMEGIVEN(client.getNickname()));
		return;
	}

	std::string targetNick = args[0];
	Client     *target = _server->getClientByNickname(targetNick);

	if (!target) {
		_server->sendReply(client,
		                   ERR_NOSUCHNICK(client.getNickname(), targetNick));
		return;
	}

	_server->sendReply(client, RPL_WHOISUSER(client.getNickname(), targetNick,
	                                         target->getUsername(),
	                                         target->getHostname(),
	                                         target->getRealname()));
	_server->sendReply(client, RPL_WHOISSERVER(client.getNickname(), targetNick,
	                                           std::string("ircserv"),
	                                           std::string("ft_irc server")));
	_server->sendReply(client,
	                   RPL_ENDOFWHOIS(client.getNickname(), targetNick));
}

// ═══════════════════════════════════════════════════════════════
//  LIST
// ═══════════════════════════════════════════════════════════════
void CommandHandler::_handleList(Client                         &client,
                                 const std::vector<std::string> &args) {
	_server->sendReply(client, RPL_LISTSTART(client.getNickname()));

	std::map<std::string, Channel *> &channels = _server->getChannels();
	for (std::map<std::string, Channel *>::iterator it = channels.begin();
	     it != channels.end(); ++it) {
		Channel *channel = it->second;

		if (!args.empty() && args[0] != channel->getName())
			continue;

		std::stringstream ss;
		ss << channel->getMemberCount();
		_server->sendReply(client,
		                   RPL_LIST(client.getNickname(), channel->getName(),
		                            ss.str(), channel->getTopic()));
	}

	_server->sendReply(client, RPL_LISTEND(client.getNickname()));
}
