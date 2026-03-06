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

	void               setNickname(const std::string &nick);
	const std::string &getNickname() const;
	void               setUsername(const std::string &user);
	const std::string &getUsername() const;
	void               setRealname(const std::string &name);
	const std::string &getRealname() const;
	int                getFd() const;

	// Registration state
	void setHasPass(bool value);
	bool hasPass() const;
	bool hasNick() const;
	bool hasUser() const;
	void setRegistered(bool value);
	bool isRegistered() const;

	// Prefix for IRC messages
	std::string getPrefix() const;

  private:
	int         _fd;
	std::string _buffer;

	std::string _nickname;
	std::string _username;
	std::string _realname;
	bool        _has_pass;
	bool        _is_registered;

	Client();
	Client(Client const &src);
	Client &operator=(Client const &rhs);
};

#endif
