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

	// Future attributes like _nickname, _username, _is_authenticated will go
	// here
};

#endif