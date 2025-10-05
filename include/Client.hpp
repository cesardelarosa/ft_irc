#pragma once

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
  public:
	Client(int fd);
	~Client();

	void         addToBuffer(const char *data, int nbytes);
	std::string &getBuffer();

	void setNickname(const std::string &nick);
	const std::string &getNickname() const;
	void setUsername(const std::string &user);
	const std::string &getUsername() const;
	void setAuthenticated(bool value);
	bool isAuthenticated() const;
	int  getFd() const;

  private:
	int         _fd;
	std::string _buffer;

	std::string _nickname;
	std::string _username;
	bool        _is_authenticated;

	Client();
	Client(Client const &src);
	Client &operator=(Client const &rhs);
};

#endif
