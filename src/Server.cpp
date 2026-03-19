#include "Server.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <sstream>
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
 * @details Cleans up all client objects, all channel objects, and closes the
 * main server socket.
 */
Server::~Server() {
	for (std::map<int, Client *>::iterator it = this->_clients.begin();
	     it != this->_clients.end(); ++it) {
		delete it->second;
	}
	for (std::map<std::string, Channel *>::iterator it =
	         this->_channels.begin();
	     it != this->_channels.end(); ++it) {
		delete it->second;
	}
	if (this->_server_fd != -1) {
		close(this->_server_fd);
	}
}

/**
 * @brief Starts the server's execution.
 */
void Server::start() {
	_setupServerSocket();
	_runEventLoop();
}

/**
 * @brief Sends a reply message to a specific client.
 * @details The message is prefixed with the server name and appended with
 * "\\r\\n".
 * @param client The client to whom the reply should be sent.
 * @param message The content of the message to send.
 */
void Server::sendReply(const Client &client, const std::string &message) {
	std::string final_message =
	    ":" + std::string("ircserv") + " " + message + "\r\n";
	if (send(client.getFd(), final_message.c_str(), final_message.length(), 0) <
	    0) {
		std::cerr << "Error sending reply to client " << client.getFd()
		          << std::endl;
	}
}

// ──────────────────────────── Accessors ─────────────────────────

const std::string &Server::getPassword() const {
	return this->_password;
}

std::map<int, Client *> &Server::getClients() {
	return this->_clients;
}

/**
 * @brief Finds a client by their nickname.
 * @param nick The nickname to search for.
 * @return Pointer to the Client, or NULL if not found.
 */
Client *Server::getClientByNickname(const std::string &nick) {
	for (std::map<int, Client *>::iterator it = this->_clients.begin();
	     it != this->_clients.end(); ++it) {
		if (it->second->getNickname() == nick) {
			return it->second;
		}
	}
	return NULL;
}

/**
 * @brief Gets a channel by its name.
 * @param name The name of the channel.
 * @return Pointer to the Channel, or NULL if not found.
 */
Channel *Server::getChannel(const std::string &name) {
	std::map<std::string, Channel *>::iterator it = this->_channels.find(name);
	if (it != this->_channels.end()) {
		return it->second;
	}
	return NULL;
}

/**
 * @brief Creates a new channel with the given name.
 * @param name The name of the new channel.
 * @return Pointer to the newly created Channel.
 */
Channel *Server::createChannel(const std::string &name) {
	Channel *channel = new Channel(name);
	this->_channels.insert(std::make_pair(name, channel));
	return channel;
}

/**
 * @brief Removes a channel and deallocates its memory.
 * @param name The name of the channel to remove.
 */
void Server::removeChannel(const std::string &name) {
	std::map<std::string, Channel *>::iterator it = this->_channels.find(name);
	if (it != this->_channels.end()) {
		delete it->second;
		this->_channels.erase(it);
	}
}

/**
 * @brief Removes a client from all channels they are in.
 * @details Also cleans up empty channels after the client leaves.
 * @param client The client to remove from all channels.
 */
void Server::removeClientFromAllChannels(Client *client) {
	std::vector<std::string> empty_channels;

	for (std::map<std::string, Channel *>::iterator it =
	         this->_channels.begin();
	     it != this->_channels.end(); ++it) {
		if (it->second->isMember(client)) {
			// Broadcast QUIT to all channel members
			it->second->broadcastMessage(
			    ":" + client->getPrefix() + " QUIT :Client quit", client);
			it->second->removeMember(client);
			if (it->second->isEmpty()) {
				empty_channels.push_back(it->first);
			}
		}
	}

	// Remove empty channels
	for (size_t i = 0; i < empty_channels.size(); ++i) {
		removeChannel(empty_channels[i]);
	}
}

// ──────────────────────────── Network ───────────────────────────

/**
 * @brief Sets up the main server listening socket.
 * @details Uses getaddrinfo for portable address resolution.
 * @throw std::runtime_error if any socket operation fails.
 */
void Server::_setupServerSocket() {
	struct addrinfo hints, *res;
	int             opt = 1;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	std::stringstream ss;
	ss << this->_port;
	std::string port_str = ss.str();

	int status = getaddrinfo(NULL, port_str.c_str(), &hints, &res);
	if (status != 0)
		throw std::runtime_error(std::string("getaddrinfo: ") +
		                         gai_strerror(status));

	this->_server_fd =
	    socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (this->_server_fd == -1) {
		freeaddrinfo(res);
		throw std::runtime_error("Failed to create socket.");
	}

	if (setsockopt(this->_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
	               sizeof(opt)) == -1) {
		freeaddrinfo(res);
		throw std::runtime_error("Failed to set socket options.");
	}

	if (fcntl(this->_server_fd, F_SETFL, O_NONBLOCK) == -1) {
		freeaddrinfo(res);
		throw std::runtime_error("Failed to set socket to non-blocking.");
	}

	if (bind(this->_server_fd, res->ai_addr, res->ai_addrlen) == -1) {
		freeaddrinfo(res);
		throw std::runtime_error("Failed to bind to port.");
	}

	freeaddrinfo(res);

	if (listen(this->_server_fd, 10) == -1)
		throw std::runtime_error("Failed to listen on socket.");

	struct pollfd server_poll_fd;
	server_poll_fd.fd = this->_server_fd;
	server_poll_fd.events = POLLIN;
	this->_fds.push_back(server_poll_fd);
}

/**
 * @brief Runs the main event loop for the server.
 * @details Exits cleanly when g_shutdown is set (via SIGINT/SIGTERM).
 * @throw std::runtime_error if poll() fails for reasons other than a signal.
 */
void Server::_runEventLoop() {
	std::cout << "Server is listening on port " << this->_port << "..."
	          << std::endl;

	while (!g_shutdown) {
		if (poll(this->_fds.data(), this->_fds.size(), -1) == -1) {
			if (errno == EINTR)
				break;
			throw std::runtime_error("poll() failed.");
		}

		if (this->_fds[0].revents & POLLIN)
			_handleNewConnection();

		for (size_t i = 1; i < this->_fds.size(); ++i) {
			if (this->_fds[i].revents & POLLIN)
				_handleClientData(i);
		}
	}

	std::cout << "\nServer shutting down gracefully." << std::endl;
}

/**
 * @brief Handles a new client connection request.
 */
void Server::_handleNewConnection() {
	struct sockaddr_storage client_addr;
	socklen_t               addr_len = sizeof(client_addr);

	int client_fd =
	    accept(this->_server_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (client_fd == -1) {
		std::cerr << "Warning: accept() failed." << std::endl;
		return;
	}

	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) {
		std::cerr << "Warning: fcntl() failed on client fd." << std::endl;
		close(client_fd);
		return;
	}

	// Resolve client IP address using inet_ntop
	char ip_str[INET6_ADDRSTRLEN];
	if (client_addr.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
		inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof(ip_str));
	} else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_addr;
		inet_ntop(AF_INET6, &s->sin6_addr, ip_str, sizeof(ip_str));
	}

	struct pollfd client_poll_fd;
	client_poll_fd.fd = client_fd;
	client_poll_fd.events = POLLIN;
	this->_fds.push_back(client_poll_fd);

	Client *new_client = new Client(client_fd);
	new_client->setHostname(ip_str);
	this->_clients.insert(std::make_pair(client_fd, new_client));

	std::cout << "New connection accepted. Client fd: " << client_fd
	          << " IP: " << ip_str << std::endl;
}

/**
 * @brief Handles incoming data from an existing client.
 * @param client_idx The index in the _fds vector.
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
 * @param client The client whose buffer needs to be processed.
 */
void Server::_processClientCommands(Client &client) {
	std::string &buffer = client.getBuffer();
	size_t       pos = 0;

	while ((pos = buffer.find("\r\n")) != std::string::npos) {
		std::string command_line = buffer.substr(0, pos);
		buffer.erase(0, pos + 2);

		if (!command_line.empty()) {
			std::cout << "Socket " << client.getFd() << " | C: " << command_line
			          << std::endl;
			this->_commandHandler.handleCommand(client, command_line);
		}
	}
}

/**
 * @brief Removes a client from the server.
 * @details Cleans up channels, closes socket, and deallocates client.
 * @param client_idx The index in the _fds vector.
 */
void Server::_removeClient(size_t client_idx) {
	int client_fd = this->_fds[client_idx].fd;

	// Remove client from all channels first
	std::map<int, Client *>::iterator it = this->_clients.find(client_fd);
	if (it != this->_clients.end()) {
		removeClientFromAllChannels(it->second);
	}

	delete this->_clients[client_fd];
	this->_clients.erase(client_fd);

	close(client_fd);
	this->_fds.erase(this->_fds.begin() + client_idx);

	std::cout << "Client " << client_fd << " removed." << std::endl;
}
