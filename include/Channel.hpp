#pragma once

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>

class Client;

class Channel {
  public:
	Channel(std::string const &name);
	~Channel();

  private:
	std::string _name;

	Channel();
	Channel(Channel const &src);
	Channel &operator=(Channel const &rhs);
};

#endif
