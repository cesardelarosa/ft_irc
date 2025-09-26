#include "Server.hpp"
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// Constructor: Initializes server attributes.
Server::Server(int port, std::string password)
    : _port(port), _password(password), _server_fd(-1) {
	std::cout << "Server created on port: " << this->_port << std::endl;
}

// Destructor: Cleans up resources.
Server::~Server() {
	for (std::map<int, Client *>::iterator it = this->_clients.begin();
	     it != this->_clients.end(); ++it) {
		delete it->second;
	}
	if (this->_server_fd != -1) {
		close(this->_server_fd);
	}
}

// Orchestrates the server startup.
void Server::start() {
	_setupServerSocket();
	_runEventLoop();
}

// Sets up the server socket.
void Server::_setupServerSocket() {
	sockaddr_in address;
	int         opt = 1;

	this->_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_server_fd == -1)
		throw std::runtime_error("Failed to create socket.");

	if (setsockopt(this->_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
	               sizeof(opt)) == -1)
		throw std::runtime_error("Failed to set socket options.");

	if (fcntl(this->_server_fd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("Failed to set socket to non-blocking.");

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(this->_port);
	if (bind(this->_server_fd, (struct sockaddr *)&address, sizeof(address)) ==
	    -1)
		throw std::runtime_error("Failed to bind to port.");

	if (listen(this->_server_fd, 10) == -1)
		throw std::runtime_error("Failed to listen on socket.");

	struct pollfd server_poll_fd;
	server_poll_fd.fd = this->_server_fd;
	server_poll_fd.events = POLLIN;
	this->_fds.push_back(server_poll_fd);
}

// Runs the main server event loop.
void Server::_runEventLoop() {
	std::cout << "Server is listening on port " << this->_port << "..."
	          << std::endl;

	while (true) {
		if (poll(this->_fds.data(), this->_fds.size(), -1) == -1)
			throw std::runtime_error("poll() failed.");

		if (this->_fds[0].revents & POLLIN)
			_handleNewConnection();

		for (size_t i = 1; i < this->_fds.size(); ++i) {
			if (this->_fds[i].revents & POLLIN)
				_handleClientData(i);
		}
	}
}

// Handles a new client connection.
void Server::_handleNewConnection() {
	int client_fd = accept(this->_server_fd, NULL, NULL);
	if (client_fd == -1) {
		std::cerr << "Warning: accept() failed." << std::endl;
		return;
	}

	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) {
		std::cerr << "Warning: fcntl() failed on client fd." << std::endl;
		close(client_fd);
		return;
	}

	struct pollfd client_poll_fd;
	client_poll_fd.fd = client_fd;
	client_poll_fd.events = POLLIN;
	this->_fds.push_back(client_poll_fd);

	this->_clients.insert(std::make_pair(client_fd, new Client(client_fd)));

	std::cout << "New connection accepted. Client fd: " << client_fd
	          << std::endl;
}

// Handles data received from a client.
void Server::_handleClientData(size_t client_idx) {
	char buffer[512];
	int  client_fd = this->_fds[client_idx].fd;

	int nbytes = recv(client_fd, buffer, sizeof(buffer), 0);

	if (nbytes <= 0) {
		if (nbytes == 0)
			std::cout << "Client " << client_fd << " disconnected."
			          << std::endl;
		else
			std::cerr << "Error: recv() failed for client " << client_fd
			          << std::endl;

		_removeClient(client_idx);
	} else {
		buffer[nbytes] = '\0';
		std::cout << "Received from client " << client_fd << ": " << buffer;
	}
}

// Removes a client from all server data structures.
void Server::_removeClient(size_t client_idx) {
	int client_fd = this->_fds[client_idx].fd;

	delete this->_clients[client_fd];
	this->_clients.erase(client_fd);

	close(client_fd);
	this->_fds.erase(this->_fds.begin() + client_idx);

	std::cout << "Client " << client_fd << " removed." << std::endl;
}
