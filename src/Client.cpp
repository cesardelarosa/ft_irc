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
