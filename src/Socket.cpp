#include "Socket.hpp"
#include "Constants.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

Socket::Socket() : _fd(-1) {}

Socket::Socket(int fd) : _fd(fd) {}

Socket::~Socket() {
  if (this->_fd != -1) {
    close(this->_fd);
  }
}

int Socket::get() const { return this->_fd; }

void Socket::setNonBlocking() {
  if (this->_fd == -1)
    return;
  if (fcntl(this->_fd, F_SETFL, O_NONBLOCK) == -1) {
    throw std::runtime_error("Failed to set socket to non-blocking.");
  }
}

void Socket::initServer(int port) {
  struct addrinfo hints, *res;
  int opt = 1;

  // 1. Resolve the local address for the given port (IPv4/TCP/passive)
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  std::stringstream ss;
  ss << port;

  int status = getaddrinfo(NULL, ss.str().c_str(), &hints, &res);
  if (status != 0)
    throw std::runtime_error("Failed to resolve host address");

  // 2. Try each resolved address until one successfully binds
  struct addrinfo *p;
  for (p = res; p != NULL; p = p->ai_next) {
    this->_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (this->_fd == -1)
      continue;

    // Allow port reuse so rapid restarts don't fail with EADDRINUSE
    if (setsockopt(this->_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
        -1) {
      close(this->_fd);
      this->_fd = -1;
      continue;
    }

    try {
      setNonBlocking();
    } catch (...) {
      close(this->_fd);
      this->_fd = -1;
      freeaddrinfo(res);
      throw;
    }

    if (bind(this->_fd, p->ai_addr, p->ai_addrlen) == 0) {
      break; /* Success */
    }

    close(this->_fd);
    this->_fd = -1;
  }

  freeaddrinfo(res);

  // All addresses exhausted without a successful bind
  if (p == NULL) {
    throw std::runtime_error("Failed to bind to port.");
  }

  // 3. Start listening for incoming connections
  if (listen(this->_fd, IRC::Limits::LISTEN_BACKLOG) == -1)
    throw std::runtime_error("Failed to listen on socket.");
}

int Socket::acceptClient(std::string &ip_str) {
  struct sockaddr_storage client_addr;
  socklen_t addr_len = static_cast<socklen_t>(sizeof(client_addr));

  int client_fd =
      accept(this->_fd, reinterpret_cast<sockaddr *>(&client_addr), &addr_len);
  if (client_fd == -1)
    return -1;

  // IPv4 connection: extract the IP string and return the fd
  if (client_addr.ss_family == AF_INET) {
    struct sockaddr_in *s = reinterpret_cast<sockaddr_in *>(&client_addr);
    ip_str = inet_ntoa(s->sin_addr);
    return client_fd;
  }

  // Non-IPv4 (e.g. IPv6): reject with -2 so the caller can log it
  close(client_fd);
  return -2;
}
