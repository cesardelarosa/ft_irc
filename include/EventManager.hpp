#pragma once

#ifndef EVENTMANAGER_HPP
#define EVENTMANAGER_HPP

#include <cstddef>
#include <poll.h>
#include <vector>

class EventManager {
  public:
	EventManager();
	~EventManager();

	void addSocket(int fd, short events);
	void removeSocket(int fd);
	void setEvents(int fd, short events);

	int    waitEvents(int timeout);
	int    getFd(size_t index) const;
	short  getRevents(size_t index) const;
	size_t getSocketCount() const;

  private:
	std::vector<struct pollfd> _fds;

	EventManager(const EventManager &src);
	EventManager &operator=(const EventManager &rhs);
};

#endif
