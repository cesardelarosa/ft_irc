#include "Server.hpp"
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

/**
 * @brief Constructs a new Server object.
 * @param port The port number for the server to listen on.
 * @param password The password required for clients to connect.
 */
Server::Server(int port, std::string password)
    : _port(port), _password(password), _server_fd(-1), _commandHandler(this) {
	std::cout << "Server created on port: " << this->_port << std::endl;
}

/**
 * @brief Destroys the Server object.
 * @details Cleans up all client objects and closes the main server socket.
 */
Server::~Server() {
	for (std::map<int, Client *>::iterator it = this->_clients.begin();
	     it != this->_clients.end(); ++it) {
		delete it->second;
	}
	if (this->_server_fd != -1) {
		close(this->_server_fd);
	}
}

/**
 * @brief Starts the server's execution.
 * @details This is the main entry point after server creation. It sets up the
 * listening socket and enters the main event loop.
 */
void Server::start() {
	_setupServerSocket();
	_runEventLoop();
}

/**
 * @brief Sends a reply message to a specific client.
 * @details The message is appended with a "\r\n" sequence before being sent.
 * @param client The client to whom the reply should be sent.
 * @param message The content of the message to send.
 */
void Server::sendReply(const Client &client, const std::string &message) {
	std::string final_message = message + "\r\n";
	if (send(client.getFd(), final_message.c_str(), final_message.length(), 0) <
	    0) {
		std::cerr << "Error sending reply to client " << client.getFd()
		          << std::endl;
	}
}

/**
 * @brief Sets up the main server listening socket.
 * @details Configures the socket, sets it to non-blocking, binds it to the
 * specified port, and starts listening for connections.
 * @throw std::runtime_error if any of the socket operations (socket,
 * setsockopt, fcntl, bind, listen) fail.
 */
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

/**
 * @brief Runs the main event loop for the server.
 * @details Uses poll() to monitor all active sockets (server and clients) for
 * incoming data or new connections.
 * @throw std::runtime_error if poll() fails.
 */
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

/**
 * @brief Handles a new client connection request.
 * @details Accepts the connection, sets the new client socket to non-blocking,
 * and adds the client to the server's management structures.
 */
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

/**
 * @brief Handles incoming data from an existing client.
 * @details Reads data from the socket, adds it to the client's buffer, and
 * then processes any complete commands found in the buffer. If the client
 * disconnected, they are removed.
 * @param client_idx The index of the client in the server's file descriptor
 * list.
 */
void Server::_handleClientData(size_t client_idx) {
	char buffer[512];
	int  client_fd = this->_fds[client_idx].fd;

	int nbytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

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
		std::map<int, Client *>::iterator it = this->_clients.find(client_fd);
		if (it != this->_clients.end()) {
			it->second->addToBuffer(buffer, nbytes);
			_processClientCommands(*it->second);
		}
	}
}

/**
 * @brief Processes the command buffer for a client.
 * @details Scans the client's buffer for command lines delimited by "\r\n",
 * extracting and dispatching each one to the CommandHandler. Any partial
 * command remains in the buffer for the next read.
 * @param client The client whose buffer needs to be processed.
 */
void Server::_processClientCommands(Client &client) {
	std::string &buffer = client.getBuffer();
	size_t       pos = 0;

	// Process all complete commands in the buffer
	while ((pos = buffer.find("\r\n")) != std::string::npos) {
		std::string command_line = buffer.substr(0, pos);
		// Erase the command and the "\r\n" from the buffer
		buffer.erase(0, pos + 2);

		if (!command_line.empty()) {
			std::cout << "Socket " << client.getFd() << " | C: " << command_line
			          << std::endl;
			this->_commandHandler.handleCommand(client, command_line);
		}
	}
}

/**
 * @brief Removes a client from the server's management structures.
 * @details Closes the client's socket, deallocates the Client object, and
 * removes the client from the fd list and client map.
 * @param client_idx The index of the client in the server's file descriptor
 * list.
 */
void Server::_removeClient(size_t client_idx) {
	int client_fd = this->_fds[client_idx].fd;

	delete this->_clients[client_fd];
	this->_clients.erase(client_fd);

	close(client_fd);
	this->_fds.erase(this->_fds.begin() + client_idx);

	std::cout << "Client " << client_fd << " removed." << std::endl;
}