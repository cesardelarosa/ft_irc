#pragma once

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>

class Socket {
  public:
	Socket();
	Socket(int fd);
	~Socket();

	void initServer(int port);
	void setNonBlocking();
	int  acceptClient(std::string &ip_str);
	int  get() const;

  private:
	int _fd;

	Socket(const Socket &src);
	Socket &operator=(const Socket &rhs);
};

#endif
