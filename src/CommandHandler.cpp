#include "CommandHandler.hpp"
#include "Channel.hpp"
#include "Constants.hpp"
#include "Replies.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>

CommandHandler::CommandHandler(Server *server)
    : _server(server), _bot(server), _commands() {
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

CommandHandler::~CommandHandler() {}

void CommandHandler::handleCommand(Client &client, std::string const &message) {
  _parseAndExecute(client, message);
}

void CommandHandler::_parseAndExecute(Client &client,
                                      const std::string &raw_command) {
  std::string line = raw_command;
  std::string trailing;
  bool has_trailing = false;

  // Strip optional IRC source prefix (starts with ':')
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
  std::stringstream ss(line);
  std::string token;
  std::vector<std::string> tokens;
  while (ss >> token) {
    tokens.push_back(token);
  }

  if (tokens.empty()) {
    return;
  }

  // 3. Separate command and arguments (commands are case-insensitive)
  std::string command = toUpper(tokens[0]);
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
      _server->sendReply(client,
                         ERR_UNKNOWNCOMMAND(client.getNickname(), command));
    }
  }
}

// ═══════════════════════════════════════════════════════════════
//  Registration helpers
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_tryRegister(Client &client) {
  if (client.isRegistered())
    return;
  if (!client.hasPass() || !client.hasNick() || !client.hasUser())
    return;

  client.setRegistered(true);
  _server->sendReply(client,
                     RPL_WELCOME(client.getNickname(), client.getUsername(),
                                 client.getHostname()));
  _server->sendReply(
      client, RPL_ISUPPORT(client.getNickname(),
                           "CHANMODES=i,t,k,l,o PREFIX=(o)@ MAXNICKLEN=9"));
  std::cout << LOG_SERVER << LOG_NICK << client.getNickname() << LOG_R
            << " registered " << ANSI_DIM << "(fd " << client.getFd() << ")"
            << LOG_R << std::endl;
}

// ═══════════════════════════════════════════════════════════════
//  PASS
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handlePass(Client &client,
                                 const std::vector<std::string> &args) {
  if (client.isRegistered()) {
    _server->sendReply(client, ERR_ALREADYREGISTRED(client.getNickname()));
    return;
  }
  if (args.empty()) {
    _server->sendReply(client,
                       ERR_NEEDMOREPARAMS(client.getNickname(), "PASS"));
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

void CommandHandler::_handleNick(Client &client,
                                 const std::vector<std::string> &args) {
  if (args.empty()) {
    std::string current =
        client.getNickname().empty() ? std::string("*") : client.getNickname();
    _server->sendReply(client, ERR_NONICKNAMEGIVEN(current));
    return;
  }

  std::string nick = args[0];
  std::string current =
      client.getNickname().empty() ? std::string("*") : client.getNickname();

  // 1. Validate first character per RFC 2812: must be a letter or special char
  if (nick.empty() || nick.length() > IRC::Limits::MAX_NICK_LENGTH ||
      (!std::isalpha(nick[0]) &&
       std::string("[]\\`_^{|}").find(nick[0]) == std::string::npos)) {
    _server->sendReply(client, ERR_ERRONEUSNICKNAME(current, nick));
    return;
  }
  // 2. Validate remaining characters: alphanumeric, hyphen, or special chars
  for (size_t j = 1; j < nick.size(); ++j) {
    char c = nick[j];
    if (!std::isalnum(c) && c != '-' &&
        std::string("[]\\`_^{|}").find(c) == std::string::npos) {
      _server->sendReply(client, ERR_ERRONEUSNICKNAME(current, nick));
      return;
    }
  }

  // 3. Reject if the nickname clashes with the bot's reserved name
  if (_bot.isNickname(nick)) {
    _server->sendReply(client, ERR_NICKNAMEINUSE(current, nick));
    return;
  }

  // 4. Reject if another client already owns this nickname
  Client *existing = _server->getClientByNickname(nick);
  if (existing != NULL && existing != &client) {
    _server->sendReply(client, ERR_NICKNAMEINUSE(current, nick));
    return;
  }

  // 5. Apply the nick change and notify accordingly
  if (client.isRegistered()) {
    // Already registered: broadcast the change to the client and its channels
    std::string old_prefix = client.getPrefix();
    client.setNickname(nick);
    std::string msg = ":" + old_prefix + " NICK " + nick;
    _server->sendToClient(client, msg + "\r\n");

    std::map<std::string, Channel *> &channels = _server->getChannels();
    for (std::map<std::string, Channel *>::iterator it = channels.begin();
         it != channels.end(); ++it) {
      if (it->second->isMember(&client)) {
        it->second->broadcastMessage(msg, &client);
      }
    }
  } else {
    // Not yet registered: just store it and attempt registration
    client.setNickname(nick);
    _tryRegister(client);
  }

  std::cout << LOG_SERVER << ANSI_DIM << "fd " << client.getFd() << LOG_R
            << " nick -> " << LOG_NICK << nick << LOG_R << std::endl;
}

// ═══════════════════════════════════════════════════════════════
//  USER
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleUser(Client &client,
                                 const std::vector<std::string> &args) {
  if (client.isRegistered()) {
    _server->sendReply(client, ERR_ALREADYREGISTRED(client.getNickname()));
    return;
  }
  if (args.size() < 4) {
    _server->sendReply(client,
                       ERR_NEEDMOREPARAMS(client.getNickname(), "USER"));
    return;
  }

  client.setUsername(args[0]);
  client.setRealname(args[3]);

  std::cout << LOG_SERVER << ANSI_DIM << "fd " << client.getFd() << LOG_R
            << " user -> " << ANSI_BRIGHT_CYAN << args[0] << LOG_R << std::endl;
  _tryRegister(client);
}

// ═══════════════════════════════════════════════════════════════
//  JOIN
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleJoin(Client &client,
                                 const std::vector<std::string> &args) {
  if (args.empty()) {
    _server->sendReply(client,
                       ERR_NEEDMOREPARAMS(client.getNickname(), "JOIN"));
    return;
  }

  // 1. Parse comma-separated channel names and their matching keys
  std::stringstream ss(args[0]);
  std::string channel_name;

  std::vector<std::string> keys;
  if (args.size() > 1) {
    std::stringstream ss_keys(args[1]);
    std::string tmp_key;
    while (std::getline(ss_keys, tmp_key, ',')) {
      keys.push_back(tmp_key);
    }
  }

  // 2. Process each channel independently
  size_t idx = 0;
  while (std::getline(ss, channel_name, ',')) {
    if (channel_name.empty())
      continue;

    // Pair each channel with its positional key (empty if none provided)
    std::string key = (idx < keys.size()) ? keys[idx] : "";
    idx++;

    // 3. Validate channel name format (must start with # or &, no forbidden
    // chars)
    if (channel_name.length() < 2 || channel_name.length() > 50 ||
        (channel_name[0] != '#' && channel_name[0] != '&') ||
        channel_name.find_first_of(" ,\x07\r\n") != std::string::npos) {
      _server->sendReply(client,
                         ERR_NOSUCHCHANNEL(client.getNickname(), channel_name));
      continue;
    }

    Channel *channel = _server->getChannel(channel_name);
    bool is_new = (channel == NULL);

    if (is_new) {
      // 4a. Channel does not exist: create it
      channel = _server->createChannel(channel_name);
      if (channel == NULL) {
        _server->sendReply(
            client, ERR_NOSUCHCHANNEL(client.getNickname(), channel_name));
        continue;
      }
    } else {
      // 4b. Channel exists: enforce join restrictions in order
      if (channel->isMember(&client))
        continue;

      // Invite-only: reject unless the client was explicitly invited
      if (channel->isInviteOnly() &&
          !channel->isInvited(client.getNickname())) {
        _server->sendReply(
            client, ERR_INVITEONLYCHAN(client.getNickname(), channel_name));
        continue;
      }

      // Key-protected: reject if the provided key does not match
      if (channel->hasKey() && key != channel->getKey()) {
        _server->sendReply(
            client, ERR_BADCHANNELKEY(client.getNickname(), channel_name));
        continue;
      }

      // User limit: reject if the channel is already full
      if (channel->hasUserLimit() &&
          channel->getMemberCount() >= channel->getUserLimit()) {
        _server->sendReply(
            client, ERR_CHANNELISFULL(client.getNickname(), channel_name));
        continue;
      }
    }

    // 5. Add client to the channel; first joiner becomes operator
    channel->addMember(&client);
    if (is_new) {
      channel->addOperator(&client);
    }

    // Consume any pending invitation now that the client has joined
    channel->removeInvited(client.getNickname());

    // 6. Broadcast JOIN and send channel state to the new member
    channel->broadcastMessage(
        ":" + client.getPrefix() + " JOIN " + channel_name, NULL);

    if (!channel->getTopic().empty()) {
      _server->sendReply(client, RPL_TOPIC(client.getNickname(), channel_name,
                                           channel->getTopic()));
    }

    std::stringstream ss_time;
    ss_time << channel->getCreationTime();
    _server->sendReply(client, RPL_CREATIONTIME(client.getNickname(),
                                                channel_name, ss_time.str()));
    _server->sendReply(client, RPL_NAMREPLY(client.getNickname(), channel_name,
                                            channel->getMemberListString()));
    _server->sendReply(client,
                       RPL_ENDOFNAMES(client.getNickname(), channel_name));
  }
}

// ═══════════════════════════════════════════════════════════════
//  PART
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handlePart(Client &client,
                                 const std::vector<std::string> &args) {
  if (args.empty()) {
    _server->sendReply(client,
                       ERR_NEEDMOREPARAMS(client.getNickname(), "PART"));
    return;
  }

  std::string reason = (args.size() > 1) ? args[1] : "";

  std::stringstream ss(args[0]);
  std::string channel_name;

  while (std::getline(ss, channel_name, ',')) {
    if (channel_name.empty())
      continue;

    Channel *channel = _server->getChannel(channel_name);
    if (channel == NULL) {
      _server->sendReply(client,
                         ERR_NOSUCHCHANNEL(client.getNickname(), channel_name));
      continue;
    }

    if (!channel->isMember(&client)) {
      _server->sendReply(client,
                         ERR_NOTONCHANNEL(client.getNickname(), channel_name));
      continue;
    }

    // Build PART message
    std::string part_msg = ":" + client.getPrefix() + " PART " + channel_name;
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

void CommandHandler::_handlePrivmsg(Client &client,
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

  // Route to the bot if the target matches the bot's reserved nickname
  if (_bot.isNickname(target)) {
    _bot.handlePrivmsg(client.getNickname(), text);
    return;
  }

  // Target is a channel: sender must be a member to write
  if (target[0] == '#' || target[0] == '&') {
    Channel *channel = _server->getChannel(target);
    if (channel == NULL) {
      _server->sendReply(client,
                         ERR_NOSUCHCHANNEL(client.getNickname(), target));
      return;
    }
    if (!channel->isMember(&client)) {
      _server->sendReply(client,
                         ERR_CANNOTSENDTOCHAN(client.getNickname(), target));
      return;
    }

    // Broadcast to all channel members except the sender
    channel->broadcastMessage(
        ":" + client.getPrefix() + " PRIVMSG " + target + " :" + text, &client);
    // Also forward to the bot so it can react to channel commands
    _bot.handlePrivmsg(target, text);
  }
  // Target is a user: deliver directly
  else {
    Client *target_client = _server->getClientByNickname(target);
    if (target_client == NULL) {
      _server->sendReply(client, ERR_NOSUCHNICK(client.getNickname(), target));
      return;
    }

    std::string msg =
        ":" + client.getPrefix() + " PRIVMSG " + target + " :" + text + "\r\n";
    _server->sendToClient(*target_client, msg);
  }
}

// ═══════════════════════════════════════════════════════════════
//  QUIT
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleQuit(Client &client,
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

void CommandHandler::_handleInvite(Client &client,
                                   const std::vector<std::string> &args) {
  if (args.size() < 2) {
    _server->sendReply(client,
                       ERR_NEEDMOREPARAMS(client.getNickname(), "INVITE"));
    return;
  }

  std::string targetNick = args[0];
  std::string channelName = args[1];

  // 1. Validate the channel exists
  Channel *channel = _server->getChannel(channelName);
  if (!channel) {
    _server->sendReply(client,
                       ERR_NOSUCHCHANNEL(client.getNickname(), channelName));
    return;
  }

  // 2. Invoker must be on the channel
  if (!channel->isMember(&client)) {
    _server->sendReply(client,
                       ERR_NOTONCHANNEL(client.getNickname(), channelName));
    return;
  }

  // 3. Invite-only channels require operator privileges to invite
  if (channel->isInviteOnly() && !channel->isOperator(&client)) {
    _server->sendReply(client,
                       ERR_CHANOPRIVSNEEDED(client.getNickname(), channelName));
    return;
  }

  // 4. Target user must exist on the server
  Client *target = _server->getClientByNickname(targetNick);
  if (!target) {
    _server->sendReply(client,
                       ERR_NOSUCHNICK(client.getNickname(), targetNick));
    return;
  }

  // 5. Target must not already be a member of the channel
  if (channel->isMember(target)) {
    _server->sendReply(client, ERR_USERONCHANNEL(client.getNickname(),
                                                 targetNick, channelName));
    return;
  }

  // 6. All checks passed: register the invite and notify both parties
  channel->addInvited(targetNick);

  _server->sendReply(
      client, RPL_INVITING(client.getNickname(), targetNick, channelName));

  std::string msg = ":" + client.getPrefix() + " INVITE " + targetNick + " :" +
                    channelName + "\r\n";

  _server->sendToClient(*target, msg);
}

// ═══════════════════════════════════════════════════════════════
//  TOPIC
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleTopic(Client &client,
                                  const std::vector<std::string> &args) {
  if (args.empty()) {
    _server->sendReply(client,
                       ERR_NEEDMOREPARAMS(client.getNickname(), "TOPIC"));
    return;
  }

  std::string channelName = args[0];
  Channel *channel = _server->getChannel(channelName);

  if (!channel) {
    _server->sendReply(client,
                       ERR_NOSUCHCHANNEL(client.getNickname(), channelName));
    return;
  }

  // Query mode: no second argument means the client is asking for the topic
  if (args.size() == 1) {
    if (channel->getTopic().empty())
      _server->sendReply(client,
                         RPL_NOTOPIC(client.getNickname(), channelName));
    else
      _server->sendReply(client, RPL_TOPIC(client.getNickname(), channelName,
                                           channel->getTopic()));
    return;
  }

  // Set mode: client wants to change the topic — must be a member
  if (!channel->isMember(&client)) {
    _server->sendReply(client,
                       ERR_NOTONCHANNEL(client.getNickname(), channelName));
    return;
  }

  // If +t is set, only operators can change the topic
  if (channel->isTopicRestricted() && !channel->isOperator(&client)) {
    _server->sendReply(client,
                       ERR_CHANOPRIVSNEEDED(client.getNickname(), channelName));
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

void CommandHandler::_handleMode(Client &client,
                                 const std::vector<std::string> &args) {
  if (args.empty()) {
    _server->sendReply(client,
                       ERR_NEEDMOREPARAMS(client.getNickname(), "MODE"));
    return;
  }

  std::string target = args[0];

  // 1. User MODE: target is not a channel (no # or & prefix)
  if (target[0] != '#' && target[0] != '&') {
    // Users can only query/change their own modes
    if (target != client.getNickname()) {
      _server->sendReply(client, ERR_USERSDONTMATCH(client.getNickname()));
      return;
    }
    if (args.size() == 1) {
      _server->sendReply(client, RPL_UMODEIS(client.getNickname(), "+"));
      return;
    } else {
      // We don't support user mode changes, only queries
      _server->sendReply(client, ERR_UMODEUNKNOWNFLAG(client.getNickname()));
      return;
    }
  }

  // 2. Channel MODE: validate the channel exists
  std::string channelName = target;
  Channel *channel = _server->getChannel(channelName);

  if (!channel) {
    _server->sendReply(client,
                       ERR_NOSUCHCHANNEL(client.getNickname(), channelName));
    return;
  }

  // 3. Query mode (no mode string): build and return the active mode flags
  if (args.size() == 1) {
    std::string modes = "+";
    std::string modes_params = "";
    if (channel->isInviteOnly())
      modes += "i";
    if (channel->isTopicRestricted())
      modes += "t";
    if (channel->hasKey()) {
      modes += "k";
      // Only reveal the actual key to channel members; outsiders see '*'
      if (channel->isMember(&client))
        modes_params += " " + channel->getKey();
      else
        modes_params += " *";
    }
    if (channel->hasUserLimit()) {
      modes += "l";
      std::stringstream ss;
      ss << channel->getUserLimit();
      modes_params += " " + ss.str();
    }

    _server->sendReply(client,
                       RPL_CHANNELMODEIS(client.getNickname(), channelName,
                                         modes + modes_params));
    return;
  }

  // 4. Change mode: requires operator privileges
  if (!channel->isOperator(&client)) {
    _server->sendReply(client,
                       ERR_CHANOPRIVSNEEDED(client.getNickname(), channelName));
    return;
  }

  std::string modes = args[1];

  std::vector<std::string> params;
  if (args.size() > 2)
    params.insert(params.begin(), args.begin() + 2, args.end());

  // 5. Delegate to the mode application engine
  _applyModes(client, *channel, modes, params, 0);
}

void CommandHandler::_applyModes(Client &client, Channel &channel,
                                 const std::string &modes,
                                 const std::vector<std::string> &params,
                                 size_t paramIndex) {
  bool adding = true;
  std::string applied_modes;
  std::string applied_params;
  bool pending_sign = false;
  char current_sign = '+';
  std::string nick = client.getNickname();

  // Iterate over each character in the mode string (e.g. "+itk-o")
  for (size_t i = 0; i < modes.size(); i++) {
    char c = modes[i];

    // Track +/- direction
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

    // Skip redundant changes (mode already in desired state)
    bool mode_applied = false;

    switch (c) {

    // +i / -i : toggle invite-only
    case 'i':
      if (channel.isInviteOnly() != adding) {
        channel.setInviteOnly(adding);
        mode_applied = true;
      }
      break;

    // +t / -t : toggle topic restriction (only ops can change topic)
    case 't':
      if (channel.isTopicRestricted() != adding) {
        channel.setTopicRestricted(adding);
        mode_applied = true;
      }
      break;

    // +k <key> / -k [*] : set or remove channel key
    case 'k':
      if (adding) {
        if (paramIndex >= params.size())
          break;
        std::string key = params[paramIndex++];
        // Cannot overwrite an existing key — must -k first (ERR_KEYSET)
        if (channel.hasKey()) {
          _server->sendReply(client, ERR_KEYSET(nick, channel.getName()));
          break;
        }
        // Reject keys containing spaces to prevent message injection
        if (key.find(' ') != std::string::npos)
          break;
        channel.setKey(key);
        applied_params += " " + key;
        mode_applied = true;
      } else {
        // -k always consumes a parameter if provided (IRC convention)
        if (paramIndex < params.size())
          paramIndex++;
        if (channel.hasKey()) {
          std::string old_key = channel.getKey();
          channel.removeKey();
          applied_params += " " + old_key;
          mode_applied = true;
        }
      }
      break;

    // +l <limit> / -l : set or remove user limit
    case 'l':
      if (adding) {
        if (paramIndex >= params.size())
          break;
        // Strict numeric validation with overflow protection
        const char *str = params[paramIndex].c_str();
        char *endptr = NULL;
        errno = 0;
        long limit = std::strtol(str, &endptr, 10);
        paramIndex++;
        // Reject on overflow, non-positive values, trailing garbage
        if (errno == ERANGE || limit <= 0 || limit > LONG_MAX / 2 ||
            endptr == str || *endptr != '\0')
          break;
        channel.setUserLimit(static_cast<size_t>(limit));
        applied_params += " " + params[paramIndex - 1];
        mode_applied = true;
      } else {
        if (channel.hasUserLimit()) {
          channel.removeUserLimit();
          mode_applied = true;
        }
      }
      break;

    // +o <nick> / -o <nick> : grant or revoke operator status
    case 'o':
      if (paramIndex >= params.size())
        break;
      {
        std::string targetNick = params[paramIndex++];
        Client *target = _server->getClientByNickname(targetNick);
        if (!target) {
          _server->sendReply(client, ERR_NOSUCHNICK(nick, targetNick));
          break;
        }
        // Target must be a member of the channel to receive/lose op
        if (!channel.isMember(target)) {
          _server->sendReply(client, ERR_USERNOTINCHANNEL(nick, targetNick,
                                                          channel.getName()));
          break;
        }
        if (adding)
          channel.addOperator(target);
        else
          channel.removeOperator(target);
        applied_params += " " + targetNick;
        mode_applied = true;
      }
      break;

    default:
      _server->sendReply(client, ERR_UNKNOWNMODE(nick, std::string(1, c)));
      break;
    }

    // Accumulate applied flags; only emit a new sign when direction changes
    if (mode_applied) {
      if (pending_sign || applied_modes.empty()) {
        applied_modes += current_sign;
        pending_sign = false;
      }
      applied_modes += c;
    }
  }

  // Broadcast the effective mode change to all channel members (if any applied)
  if (!applied_modes.empty()) {
    std::string msg = ":" + client.getPrefix() + " MODE " + channel.getName() +
                      " " + applied_modes + applied_params;
    channel.broadcastMessage(msg, NULL);
  }
}

// ═══════════════════════════════════════════════════════════════
//  KICK
// ═══════════════════════════════════════════════════════════════
void CommandHandler::_handleKick(Client &client,
                                 const std::vector<std::string> &args) {
  if (args.size() < 2) {
    _server->sendReply(client,
                       ERR_NEEDMOREPARAMS(client.getNickname(), "KICK"));
    return;
  }

  std::string channelNamesStr = args[0];
  std::string targetNicksStr = args[1];
  std::string reason = (args.size() > 2) ? args[2] : client.getNickname();

  // 1. Parse comma-separated channel and target lists
  std::vector<std::string> channels;
  std::stringstream ssChan(channelNamesStr);
  std::string t;
  while (std::getline(ssChan, t, ',')) {
    if (!t.empty())
      channels.push_back(t);
  }

  std::vector<std::string> targets;
  std::stringstream ssTarg(targetNicksStr);
  while (std::getline(ssTarg, t, ',')) {
    if (!t.empty())
      targets.push_back(t);
  }

  if (channels.empty() || targets.empty())
    return;

  // 2. Pair targets with channels: if multiple channels, each target maps
  //    to its positional channel; otherwise all targets use the single channel
  bool multiple_channels = (channels.size() > 1);
  for (size_t j = 0; j < targets.size(); ++j) {
    std::string targetNick = targets[j];
    std::string channelName =
        multiple_channels && j < channels.size() ? channels[j] : channels[0];

    // 3. Validate channel exists and invoker has operator privileges
    Channel *channel = _server->getChannel(channelName);
    if (!channel) {
      _server->sendReply(client,
                         ERR_NOSUCHCHANNEL(client.getNickname(), channelName));
      continue;
    }
    if (!channel->isOperator(&client)) {
      _server->sendReply(
          client, ERR_CHANOPRIVSNEEDED(client.getNickname(), channelName));
      continue;
    }

    // 4. Validate target exists and is a member of the channel
    Client *target = _server->getClientByNickname(targetNick);
    if (!target || !channel->isMember(target)) {
      _server->sendReply(client, ERR_USERNOTINCHANNEL(client.getNickname(),
                                                      targetNick, channelName));
      continue;
    }

    // 5. Broadcast KICK, remove member, and clean up empty channels
    std::string msg = ":" + client.getPrefix() + " KICK " + channelName + " " +
                      targetNick + " :" + reason;
    channel->broadcastMessage(msg, NULL);
    channel->removeMember(target);
    if (channel->isEmpty())
      _server->removeChannel(channelName);
  }
}

// ═══════════════════════════════════════════════════════════════
//  PING
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handlePing(Client &client,
                                 const std::vector<std::string> &args) {
  if (args.empty()) {
    _server->sendReply(client,
                       ERR_NEEDMOREPARAMS(client.getNickname(), "PING"));
    return;
  }

  std::string msg = ":" + IRC::Identity::SERVER_NAME + " PONG " +
                    IRC::Identity::SERVER_NAME + " :" + args[0] + "\r\n";
  _server->sendToClient(client, msg);
}

// ═══════════════════════════════════════════════════════════════
//  PONG
// ═══════════════════════════════════════════════════════════════
void CommandHandler::_handlePong(Client &client,
                                 const std::vector<std::string> &args) {
  (void)client;
  (void)args;
}

// ═══════════════════════════════════════════════════════════════
//  NOTICE
// ═══════════════════════════════════════════════════════════════
void CommandHandler::_handleNotice(Client &client,
                                   const std::vector<std::string> &args) {
  if (args.empty() || args.size() < 2)
    return;

  std::string target = args[0];
  std::string text = args[1];

  // Channel target: broadcast to members (sender excluded, per RFC)
  if (target[0] == '#' || target[0] == '&') {
    Channel *channel = _server->getChannel(target);
    if (channel && channel->isMember(&client)) {
      channel->broadcastMessage(":" + client.getPrefix() + " NOTICE " + target +
                                    " :" + text,
                                &client);
    }
    // User target: deliver directly, silently drop if user not found
  } else {
    Client *target_client = _server->getClientByNickname(target);
    if (target_client) {
      std::string msg =
          ":" + client.getPrefix() + " NOTICE " + target + " :" + text + "\r\n";
      _server->sendToClient(*target_client, msg);
    }
  }
}

// ═══════════════════════════════════════════════════════════════
//  CAP
// ═══════════════════════════════════════════════════════════════

void CommandHandler::_handleCap(Client &client,
                                const std::vector<std::string> &args) {
  // CAP LS: reply with an empty capability list (we don't support any)
  if (!args.empty() && args[0] == "LS") {
    std::string msg = ":" + IRC::Identity::SERVER_NAME + " CAP * LS :\r\n";
    _server->sendToClient(client, msg);
  }
}

// ═══════════════════════════════════════════════════════════════
//  WHOIS
// ═══════════════════════════════════════════════════════════════
void CommandHandler::_handleWhois(Client &client,
                                  const std::vector<std::string> &args) {
  if (args.empty()) {
    _server->sendReply(client, ERR_NONICKNAMEGIVEN(client.getNickname()));
    return;
  }

  std::string targetNick = args[0];
  Client *target = _server->getClientByNickname(targetNick);

  if (!target) {
    _server->sendReply(client,
                       ERR_NOSUCHNICK(client.getNickname(), targetNick));
    return;
  }

  _server->sendReply(client,
                     RPL_WHOISUSER(client.getNickname(), targetNick,
                                   target->getUsername(), target->getHostname(),
                                   target->getRealname()));
  _server->sendReply(client, RPL_WHOISSERVER(client.getNickname(), targetNick,
                                             IRC::Identity::SERVER_NAME,
                                             IRC::Identity::SERVER_INFO));
  _server->sendReply(client, RPL_ENDOFWHOIS(client.getNickname(), targetNick));
}

// ═══════════════════════════════════════════════════════════════
//  LIST
// ═══════════════════════════════════════════════════════════════
void CommandHandler::_handleList(Client &client,
                                 const std::vector<std::string> &args) {
  _server->sendReply(client, RPL_LISTSTART(client.getNickname()));

  std::map<std::string, Channel *> &channels = _server->getChannels();
  for (std::map<std::string, Channel *>::iterator it = channels.begin();
       it != channels.end(); ++it) {
    Channel *channel = it->second;

    // If a specific channel was requested, skip all others
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
