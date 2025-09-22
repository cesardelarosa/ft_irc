#pragma once

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>

class Client;

class Channel {
  public:
	Channel(std::string const &name);
	~Channel(void);

  private:
	std::string _name;

	Channel(void);
	Channel(Channel const &src);
	Channel &operator=(Channel const &rhs);
};

#endif
