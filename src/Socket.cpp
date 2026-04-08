#include "Socket.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

Socket::Socket() : _fd(-1) {
}

Socket::Socket(int fd) : _fd(fd) {
}

Socket::~Socket() {
	if (this->_fd != -1) {
		close(this->_fd);
	}
}

int Socket::get() const {
	return this->_fd;
}

void Socket::setNonBlocking() {
	if (this->_fd == -1)
		return;
	if (fcntl(this->_fd, F_SETFL, O_NONBLOCK) == -1) {
		throw std::runtime_error("Failed to set socket to non-blocking.");
	}
}

void Socket::initServer(int port) {
	struct addrinfo hints, *res;
	int             opt = 1;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	std::stringstream ss;
	ss << port;

	int status = getaddrinfo(NULL, ss.str().c_str(), &hints, &res);
	if (status != 0)
		throw std::runtime_error(std::string("getaddrinfo: ") +
		                         gai_strerror(status));

	struct addrinfo *p;
	for (p = res; p != NULL; p = p->ai_next) {
		this->_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (this->_fd == -1)
			continue;

		if (setsockopt(this->_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
		               sizeof(opt)) == -1) {
			close(this->_fd);
			this->_fd = -1;
			continue;
		}

		setNonBlocking();

		if (bind(this->_fd, p->ai_addr, p->ai_addrlen) == 0) {
			break; /* Success */
		}

		close(this->_fd);
		this->_fd = -1;
	}

	freeaddrinfo(res);

	if (p == NULL) {
		throw std::runtime_error("Failed to bind to port.");
	}

	if (listen(this->_fd, 10) == -1)
		throw std::runtime_error("Failed to listen on socket.");
}

int Socket::acceptClient(std::string &ip_str) {
	struct sockaddr_storage client_addr;
	socklen_t               addr_len = sizeof(client_addr);

	int client_fd =
	    accept(this->_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (client_fd == -1)
		return -1;

	char ip[INET6_ADDRSTRLEN];
	if (client_addr.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
		inet_ntop(AF_INET, &s->sin_addr, ip, sizeof(ip));
	} else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_addr;
		inet_ntop(AF_INET6, &s->sin6_addr, ip, sizeof(ip));
	}
	ip_str = ip;

	return client_fd;
}
