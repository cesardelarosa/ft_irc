#pragma once

#ifndef BOT_HPP
#define BOT_HPP

#include <string>

class Server;

class Bot {
  public:
	Bot(Server *server);
	~Bot();

	bool isNickname(const std::string &nick) const;
	void handlePrivmsg(const std::string &target, const std::string &text);

  private:
	Server *_server;

	std::string _getReply(const std::string &text) const;
	std::string _getTime() const;
	void _sendNotice(const std::string &target, const std::string &text);

	Bot();
	Bot(Bot const &src);
	Bot &operator=(Bot const &rhs);
};

#endif
