#pragma once

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
  public:
	Client(int fd);
	~Client(void);

  private:
	int         _fd;
	std::string _buffer;

	std::string _nickname;
	std::string _username;
	bool        _is_authenticated;

	Client(void);
	Client(Client const &src);
	Client &operator=(Client const &rhs);
};

#endif
