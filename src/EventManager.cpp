#include "EventManager.hpp"

// ──────────────────────────── Constructors ────────────────────────────

EventManager::EventManager() {
}

EventManager::~EventManager() {
}

// ──────────────────────────── Socket Management ───────────────────────

void EventManager::addSocket(int fd, short events) {
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;
	this->_fds.push_back(pfd);
}

void EventManager::removeSocket(int fd) {
	for (size_t i = 0; i < this->_fds.size(); ++i) {
		if (this->_fds[i].fd == fd) {
			this->_fds[i] = this->_fds.back();
			this->_fds.pop_back();
			break;
		}
	}
}

void EventManager::setEvents(int fd, short events) {
	for (size_t i = 0; i < this->_fds.size(); ++i) {
		if (this->_fds[i].fd == fd) {
			this->_fds[i].events = events;
			break;
		}
	}
}

// ──────────────────────────── Event Loop ──────────────────────────────

int EventManager::waitEvents(int timeout) {
	if (this->_fds.empty())
		return 0;
	return poll(&this->_fds[0], this->_fds.size(), timeout);
}

// ──────────────────────────── Getters ─────────────────────────────────

int EventManager::getFd(size_t index) const {
	if (index >= this->_fds.size())
		return -1;
	return this->_fds[index].fd;
}

short EventManager::getRevents(size_t index) const {
	if (index >= this->_fds.size())
		return 0;
	return this->_fds[index].revents;
}

size_t EventManager::getSocketCount() const {
	return this->_fds.size();
}
