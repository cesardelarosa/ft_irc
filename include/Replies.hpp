#pragma once

#ifndef REPLIES_HPP
#define REPLIES_HPP

#include <string>

// ──────────────────────────── Success Replies ────────────────────────────

#define RPL_WELCOME(nick, user, host)                                        \
	"001 " + nick + " :Welcome to the Internet Relay Chat Network " + nick + \
	    "!" + user + "@" + host

#define RPL_NOTOPIC(nick, channel) \
	"331 " + nick + " " + channel + " :No topic is set"

#define RPL_TOPIC(nick, channel, topic) \
	"332 " + nick + " " + channel + " :" + topic

#define RPL_INVITING(nick, target, channel) \
	"341 " + nick + " " + target + " " + channel

#define RPL_NAMREPLY(nick, channel, names) \
	"353 " + nick + " = " + channel + " :" + names

#define RPL_ENDOFNAMES(nick, channel) \
	"366 " + nick + " " + channel + " :End of /NAMES list"

// ──────────────────────────── Error Replies ─────────────────────────────

#define ERR_NOSUCHNICK(nick, target) \
	"401 " + nick + " " + target + " :No such nick/channel"

#define ERR_NOSUCHCHANNEL(nick, channel) \
	"403 " + nick + " " + channel + " :No such channel"

#define ERR_CANNOTSENDTOCHAN(nick, channel) \
	"404 " + nick + " " + channel + " :Cannot send to channel"

#define ERR_NORECIPIENT(nick) "411 " + nick + " :No recipient given (PRIVMSG)"

#define ERR_NOTEXTTOSEND(nick) "412 " + nick + " :No text to send"

#define ERR_UNKNOWNCOMMAND(command) \
	std::string("421 ") + command + " :Unknown command"

#define ERR_NONICKNAMEGIVEN(nick) "431 " + nick + " :No nickname given"

#define ERR_ERRONEUSNICKNAME(nick, badnick) \
	"432 " + nick + " " + badnick + " :Erroneous nickname"

#define ERR_NICKNAMEINUSE(nick, badnick) \
	"433 " + nick + " " + badnick + " :Nickname is already in use"

#define ERR_USERNOTINCHANNEL(nick, target, channel) \
	"441 " + nick + " " + target + " " + channel +  \
	    " :They aren't on that channel"

#define ERR_NOTONCHANNEL(nick, channel) \
	"442 " + nick + " " + channel + " :You're not on that channel"

#define ERR_USERONCHANNEL(nick, target, channel) \
	"443 " + nick + " " + target + " " + channel + " :is already on channel"

#define ERR_NOTREGISTERED "451 * :You have not registered"

#define ERR_NEEDMOREPARAMS(command) \
	std::string("461 ") + command + " :Not enough parameters"

#define ERR_ALREADYREGISTRED "462 :Unauthorized command (already registered)"

#define ERR_PASSWDMISMATCH "464 :Password incorrect"

#define ERR_CHANNELISFULL(nick, channel) \
	"471 " + nick + " " + channel + " :Cannot join channel (+l)"

#define ERR_INVITEONLYCHAN(nick, channel) \
	"473 " + nick + " " + channel + " :Cannot join channel (+i)"

#define ERR_BADCHANNELKEY(nick, channel) \
	"475 " + nick + " " + channel + " :Cannot join channel (+k)"

#define ERR_CHANOPRIVSNEEDED(nick, channel) \
	"482 " + nick + " " + channel + " :You're not channel operator"

#endif
