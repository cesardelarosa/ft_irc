#pragma once

#ifndef REPLIES_HPP
#define REPLIES_HPP

// ──────────────────────────── SUCCESS ────────────────────────────

#define RPL_WELCOME(nick, user, host)                                          \
  "001 " + nick + " :Welcome to the Internet Relay Chat Network " + nick +     \
      "!" + user + "@" + host

// CAPABILITIES
#define RPL_ISUPPORT(nick, tokens)                                             \
  "005 " + nick + " " + tokens + " :are supported by this server"

#define RPL_NOTOPIC(nick, channel)                                             \
  "331 " + nick + " " + channel + " :No topic is set"

#define RPL_TOPIC(nick, channel, topic)                                        \
  "332 " + nick + " " + channel + " :" + topic

#define RPL_INVITING(nick, target, channel)                                    \
  "341 " + nick + " " + target + " " + channel

#define RPL_NAMREPLY(nick, channel, names)                                     \
  "353 " + nick + " = " + channel + " :" + names

#define RPL_ENDOFNAMES(nick, channel)                                          \
  "366 " + nick + " " + channel + " :End of /NAMES list"

#define RPL_CHANNELMODEIS(nick, channel, modes)                                \
  "324 " + nick + " " + channel + " " + modes

#define RPL_ENDOFINVITELIST(nick, channel)                                     \
  "337 " + nick + " " + channel + " :End of INVITE list"

// CHANNEL CREATION
#define RPL_CREATIONTIME(nick, channel, creation_time)                         \
  "329 " + nick + " " + channel + " " + creation_time

// WHOIS
#define RPL_WHOISUSER(nick, target, user, host, realname)                      \
  "311 " + nick + " " + target + " " + user + " " + host + " * :" + realname

#define RPL_WHOISSERVER(nick, target, server, serverinfo)                      \
  "312 " + nick + " " + target + " " + server + " :" + serverinfo

#define RPL_ENDOFWHOIS(nick, target)                                           \
  "318 " + nick + " " + target + " :End of /WHOIS list"

// LIST
#define RPL_LISTSTART(nick) "321 " + nick + " Channel :Users  Name"

#define RPL_LIST(nick, channel, count, topic)                                  \
  "322 " + nick + " " + channel + " " + count + " :" + topic

#define RPL_LISTEND(nick) "323 " + nick + " :End of /LIST"

// ──────────────────────────── ERRORS ─────────────────────────────

// GENERAL
#define ERR_UNKNOWNCOMMAND(nick, command)                                      \
  "421 " + nick + " " + command + " :Unknown command"

#define ERR_NOTREGISTERED(nick) "451 " + nick + " :You have not registered"

#define ERR_NEEDMOREPARAMS(nick, command)                                      \
  "461 " + (nick.empty() ? std::string("*") : nick) + " " + command +          \
      " :Not enough parameters"

#define ERR_ALREADYREGISTRED(nick)                                             \
  "462 " + nick + " :Unauthorized command (already registered)"

#define ERR_PASSWDMISMATCH(nick) "464 " + nick + " :Password incorrect"

// USERS
#define ERR_NOSUCHNICK(nick, target)                                           \
  "401 " + nick + " " + target + " :No such nick/channel"

#define ERR_USERNOTINCHANNEL(nick, target, channel)                            \
  "441 " + nick + " " + target + " " + channel + " :They aren't on that channel"

#define ERR_USERONCHANNEL(nick, target, channel)                               \
  "443 " + nick + " " + target + " " + channel + " :is already on channel"

// CHANNEL
#define ERR_NOSUCHCHANNEL(nick, channel)                                       \
  "403 " + nick + " " + channel + " :No such channel"

#define ERR_NOTONCHANNEL(nick, channel)                                        \
  "442 " + nick + " " + channel + " :You're not on that channel"

#define ERR_CHANOPRIVSNEEDED(nick, channel)                                    \
  "482 " + nick + " " + channel + " :You're not channel operator"

#define ERR_CHANNELISFULL(nick, channel)                                       \
  "471 " + nick + " " + channel + " :Cannot join channel (+l)"

#define ERR_INVITEONLYCHAN(nick, channel)                                      \
  "473 " + nick + " " + channel + " :Cannot join channel (+i)"

#define ERR_BADCHANNELKEY(nick, channel)                                       \
  "475 " + nick + " " + channel + " :Cannot join channel (+k)"

#define ERR_KEYSET(nick, channel)                                              \
  "467 " + nick + " " + channel + " :Channel key already set"

// PRIVMSG
#define ERR_NORECIPIENT(nick) "411 " + nick + " :No recipient given (PRIVMSG)"

#define ERR_NOTEXTTOSEND(nick) "412 " + nick + " :No text to send"

#define ERR_CANNOTSENDTOCHAN(nick, channel)                                    \
  "404 " + nick + " " + channel + " :Cannot send to channel"

// MODE
#define ERR_UNKNOWNMODE(nick, mode)                                            \
  "472 " + nick + " " + mode + " :is unknown mode char to me"

// USER MODES
#define RPL_UMODEIS(nick, modes) "221 " + nick + " " + modes

#define ERR_UMODEUNKNOWNFLAG(nick) "501 " + nick + " :Unknown MODE flag"

#define ERR_USERSDONTMATCH(nick)                                               \
  "502 " + nick + " :Cannot change mode for other users"

// NICK
#define ERR_NONICKNAMEGIVEN(nick) "431 " + nick + " :No nickname given"

#define ERR_ERRONEUSNICKNAME(nick, badnick)                                    \
  "432 " + nick + " " + badnick + " :Erroneous nickname"

#define ERR_NICKNAMEINUSE(nick, badnick)                                       \
  "433 " + nick + " " + badnick + " :Nickname is already in use"

#endif
