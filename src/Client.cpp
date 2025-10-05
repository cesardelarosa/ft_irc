#include "Client.hpp"

Client::Client(int fd) : _fd(fd), _is_authenticated(false) {
}

Client::~Client() {
}

void Client::addToBuffer(const char *data, int nbytes) {
	this->_buffer.append(data, nbytes);
}

std::string &Client::getBuffer() {
	return this->_buffer;
}

void Client::setNickname(const std::string &nick) {
	this->_nickname = nick;
}

const std::string &Client::getNickname() const {
	return this->_nickname;
}

void Client::setUsername(const std::string &user) {
	this->_username = user;
}

const std::string &Client::getUsername() const {
	return this->_username;
}

void Client::setAuthenticated(bool value) {
	this->_is_authenticated = value;
}

bool Client::isAuthenticated() const {
	return this->_is_authenticated;
}

int Client::getFd() const {
	return this->_fd;
}
