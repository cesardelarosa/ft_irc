#pragma once

#ifndef REPLIES_HPP
#define REPLIES_HPP

#include <string>

#define RPL_WELCOME(nick, user, host)                                        \
	"001 " + nick + " :Welcome to the Internet Relay Chat Network " + nick + \
	    "!" + user + "@" + host

#define ERR_UNKNOWNCOMMAND(command) "421 " + command + " :Unknown command"
#define ERR_NONICKNAMEGIVEN "421 :No nickname given"
#define ERR_NICKNAMEINUSE(nick) "433 * " + nick + " :Nickname is already in use"
#define ERR_NEEDMOREPARAMS(command) "461 " + command + " :Not enough parameters"
#define ERR_ALREADYREGISTRED "462 :Unauthorized command (already registered)"
#define ERR_PASSWDMISMATCH "464 :Password incorrect"

#endif
