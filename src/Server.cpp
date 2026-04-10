#include "Server.hpp"
#include "Utils.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

/**
 * @brief Constructs a new Server object.
 * @param port The port number for the server to listen on.
 * @param password The password required for clients to connect.
 */
Server::Server(int port, std::string password)
    : _port(port), _password(password), _clients(), _channels(),
      _serverSocket(), _eventManager(), _commandHandler(this) {
	std::cout << LOG_SERVER << "Server created on port: " << ANSI_BOLD
	          << this->_port << LOG_R << std::endl;
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
}

/**
 * @brief Starts the server by setting up the socket and entering the event
 * loop.
 */
void Server::start() {
	_setupServerSocket();
	_runEventLoop();
}

/**
 * @brief Sends a standardized IRC reply to a client without modifying its queue
 * logic directly.
 * @param client The target client.
 * @param message The reply string to send.
 */
void Server::sendReply(Client &client, const std::string &message) {
	std::string final_message =
	    ":" + std::string("ircserv") + " " + message + "\r\n";
	client.queueMessage(final_message);
}

/**
 * @brief Directly appends a raw formatted message into the client's send
 * buffer.
 * @param client The target client.
 * @param message The message to enqueue.
 */
void Server::sendToClient(Client &client, const std::string &message) {
	client.queueMessage(message);
}

// ──────────────────────────── Accessors ─────────────────────────

const std::string &Server::getPassword() const {
	return this->_password;
}

std::map<int, Client *> &Server::getClients() {
	return this->_clients;
}

std::map<std::string, Channel *> &Server::getChannels() {
	return this->_channels;
}

/**
 * @brief Finds a client by their nickname.
 * @param nick The nickname to search for.
 * @return Pointer to the Client, or NULL if not found.
 */
Client *Server::getClientByNickname(const std::string &nick) {
	std::string lower_nick = toIrcLower(nick);
	for (std::map<int, Client *>::iterator it = this->_clients.begin();
	     it != this->_clients.end(); ++it) {
		if (toIrcLower(it->second->getNickname()) == lower_nick) {
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
	std::map<std::string, Channel *>::iterator it =
	    this->_channels.find(toIrcLower(name));
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
	Channel *channel = NULL;
	try {
		channel = new Channel(name);
		this->_channels.insert(std::make_pair(toIrcLower(name), channel));
	} catch (const std::exception &) {
		std::cerr << LOG_ERROR << "Out of memory creating channel " << LOG_CHAN
		          << name << LOG_R << std::endl;
		if (channel)
			delete channel;
		return NULL;
	}
	return channel;
}

/**
 * @brief Removes a channel and deallocates its memory.
 * @param name The name of the channel to remove.
 */
void Server::removeChannel(const std::string &name) {
	std::map<std::string, Channel *>::iterator it =
	    this->_channels.find(toIrcLower(name));
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
void Server::removeClientFromAllChannels(Client            *client,
                                         const std::string &reason) {
	std::vector<std::string> empty_channels;

	for (std::map<std::string, Channel *>::iterator it =
	         this->_channels.begin();
	     it != this->_channels.end(); ++it) {
		if (it->second->isMember(client)) {
			// Broadcast QUIT to all channel members
			it->second->broadcastMessage(
			    ":" + client->getPrefix() + " QUIT :" + reason, client);
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
	this->_serverSocket.initServer(this->_port);
	this->_eventManager.addSocket(this->_serverSocket.get(), POLLIN);
}

/**
 * @brief Main execution loop polling read and write socket events.
 * @details Exits cleanly when g_shutdown is set (via SIGINT/SIGTERM),
 * manages timeouts and distributes interactions to discrete handlers.
 * @throw std::runtime_error if poll() fails for reasons other than a signal.
 */
void Server::_runEventLoop() {
	std::cout << std::endl;
	std::cout << ANSI_BOLD ANSI_CYAN
	          << " ╔══════════════════════════════════════════╗" << std::endl;
	std::cout << " ║        " ANSI_BRIGHT_WHITE "f t _ i r c   s e r v e r"
	          << ANSI_CYAN "         ║" << std::endl;
	std::cout << " ║" ANSI_RESET ANSI_CYAN "           ── C++ 98 Edition ──"
	          << "           " ANSI_BOLD "║" << std::endl;
	std::cout << " ╚══════════════════════════════════════════╝" << ANSI_RESET
	          << std::endl;
	std::cout << std::endl;
	std::cout << LOG_SERVER << "Listening on port " << ANSI_BOLD << this->_port
	          << LOG_R << " ..." << std::endl;
	std::cout << LOG_SERVER << ANSI_DIM << "Waiting for connections." << LOG_R
	          << std::endl;
	std::cout << std::endl;

	time_t last_timeout_check = std::time(NULL);

	while (!g_shutdown) {
		_updatePollEvents();

		if (this->_eventManager.waitEvents(5000) == -1) {
			if (errno == EINTR)
				break;
			throw std::runtime_error("poll() failed.");
		}

		time_t now = std::time(NULL);
		if (now - last_timeout_check >= 10) {
			_checkTimeouts();
			last_timeout_check = now;
		}

		if (this->_eventManager.getRevents(0) & POLLIN)
			_handleNewConnection();

		for (size_t i = 1; i < this->_eventManager.getSocketCount();) {
			int   fd = this->_eventManager.getFd(i);
			short revents = this->_eventManager.getRevents(i);
			bool  removed = false;

			if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
				std::cerr << LOG_ERROR << "Socket error on fd " << LOG_FD << fd
				          << LOG_R << std::endl;
				_removeClient(fd);
				removed = true;
			} else if (revents & POLLIN) {
				if (_handleClientData(fd)) {
					removed = true;
				}
			}

			if (!removed && (revents & POLLOUT)) {
				if (_handleClientWrite(fd)) {
					removed = true;
				}
			}

			if (!removed) {
				++i;
			}
		}
	}

	std::cout << std::endl;
	std::cout << LOG_SERVER << ANSI_BOLD ANSI_YELLOW
	          << "Server shutting down gracefully." << LOG_R << std::endl;
}

/**
 * @brief Accepts a new client connection natively and registers it to the event
 * manager.
 */
void Server::_handleNewConnection() {
	std::string ip_str;
	int         client_fd = this->_serverSocket.acceptClient(ip_str);

	if (client_fd == -2) {
		std::cerr << LOG_WARN << "Connection rejected: Only IPv4 is supported."
		          << std::endl;
		return;
	}

	if (client_fd == -1) {
		std::cerr << LOG_WARN << "accept() failed." << std::endl;
		return;
	}

	Client *new_client = NULL;
	try {
		new_client = new Client(client_fd);
		new_client->setHostname(ip_str);
		this->_eventManager.addSocket(client_fd, POLLIN);
		this->_clients.insert(std::make_pair(client_fd, new_client));
	} catch (const std::exception &) {
		std::cerr << LOG_ERROR << "Out of memory for new client" << std::endl;
		if (new_client)
			delete new_client;
		else
			close(client_fd);
		return;
	}

	std::cout << LOG_CONNECT << ANSI_GREEN << "New connection" << LOG_R
	          << " from " << ANSI_BOLD << ip_str << LOG_R << LOG_FD << " (fd "
	          << client_fd << ")" << LOG_R << std::endl;
}

/**
 * @brief Reads available incoming data from a client socket and processes it.
 * @param client_idx The index/fd of the client in the event manager.
 * @return True if successful, false if the client disconnected gracefully or
 * failed.
 */
bool Server::_handleClientData(int client_fd) {
	char    buffer[512];
	ssize_t nbytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

	if (nbytes < 0) {
		if (errno == EAGAIN)
			return false;
		std::cerr << LOG_ERROR << "recv() failed for fd " << LOG_FD << client_fd
		          << LOG_R << std::endl;
		_removeClient(client_fd);
		return true;
	}

	if (nbytes == 0) {
		std::cout << LOG_DISCONNECT << ANSI_RED << "Client disconnected"
		          << LOG_R << LOG_FD << " (fd " << client_fd << ")" << LOG_R
		          << std::endl;
		_removeClient(client_fd);
		return true;
	}

	buffer[nbytes] = '\0';
	std::map<int, Client *>::iterator it = this->_clients.find(client_fd);
	if (it != this->_clients.end()) {
		it->second->updateActivity();
		it->second->addToBuffer(buffer, nbytes);

		if (it->second->getBuffer().size() > 4096) {
			std::cerr << LOG_WARN << "Buffer overflow from fd " << LOG_FD
			          << client_fd << LOG_R << ANSI_YELLOW << " — disconnecting"
			          << LOG_R << std::endl;
			_removeClient(client_fd);
			return true;
		}
		_processClientCommands(*it->second);

		if (it->second->isDisconnected()) {
			if (it->second->hasPendingData()) {
				const std::string &buf = it->second->getSendBuffer();
				send(client_fd, buf.c_str(), buf.length(), 0);
			}
			_removeClient(client_fd);
			return true;
		}
	}
	return false;
}

/**
 * @brief Slices the raw client buffer into distinct CRLF-delimited commands
 * and passes them to the CommandHandler engine.
 * @param client The client entity to process.
 */
void Server::_processClientCommands(Client &client) {
	std::string &buffer = client.getBuffer();
	size_t       pos = 0;

	while ((pos = buffer.find("\r\n")) != std::string::npos) {
		std::string command_line = buffer.substr(0, pos);
		buffer.erase(0, pos + 2);

		if (command_line.length() > 510) {
			command_line = command_line.substr(0, 510);
		}

		if (!command_line.empty()) {
			std::cout << LOG_CMD << LOG_FD << "fd " << client.getFd() << LOG_R
			          << " " << ANSI_BRIGHT_WHITE << command_line << LOG_R
			          << std::endl;
			this->_commandHandler.handleCommand(client, command_line);
		}

		if (client.isDisconnected()) {
			break;
		}
	}
}

/**
 * @brief Removes a client from the server.
 * @details Cleans up channels, closes socket, and deallocates client.
 * @param client_idx The index in the _fds vector.
 */
void Server::_removeClient(int client_fd) {
	std::map<int, Client *>::iterator it = this->_clients.find(client_fd);
	if (it != this->_clients.end()) {
		removeClientFromAllChannels(it->second, it->second->getQuitReason());
		delete it->second;
		this->_clients.erase(it);
	}

	this->_eventManager.removeSocket(client_fd);

	std::cout << LOG_DISCONNECT << ANSI_DIM << "Client fd " << client_fd
	          << " removed." << LOG_R << std::endl;
}

/**
 * @brief Flushes the outbound buffer queue for a client.
 * @param client_idx The index/fd of the client in the event manager.
 * @return True if sending was successful, false if the connection dropped.
 */
bool Server::_handleClientWrite(int client_fd) {
	std::map<int, Client *>::iterator it = this->_clients.find(client_fd);
	if (it == this->_clients.end() || !it->second->hasPendingData())
		return false;

	const std::string &buf = it->second->getSendBuffer();
	ssize_t            sent = send(client_fd, buf.c_str(), buf.length(), 0);

	if (sent < 0) {
		if (errno == EAGAIN)
			return false;
		std::cerr << LOG_ERROR << "send() failed for fd " << LOG_FD << client_fd
		          << LOG_R << std::endl;
		_removeClient(client_fd);
		return true;
	}

	it->second->clearSentBytes(static_cast<size_t>(sent));
	return false;
}

/**
 * @brief Updates poll events for all client fds.
 * @details Sets POLLOUT when a client has data waiting to be sent,
 *          clears it when the send buffer is empty.
 */
void Server::_updatePollEvents() {
	for (std::map<int, Client *>::iterator it = this->_clients.begin();
	     it != this->_clients.end(); ++it) {
		int fd = it->first;
		if (it->second->hasPendingData())
			this->_eventManager.setEvents(fd, POLLIN | POLLOUT);
		else
			this->_eventManager.setEvents(fd, POLLIN);
	}
}

/**
 * @brief Iterates over clients verifying inactivity bounds to fire pings or
 * perform mass disconnections.
 */
void Server::_checkTimeouts() {
	time_t           now = std::time(NULL);
	std::vector<int> to_remove;

	for (std::map<int, Client *>::iterator it = this->_clients.begin();
	     it != this->_clients.end(); ++it) {
		time_t idle_time = now - it->second->getLastActivity();
		if (idle_time > 180) {
			to_remove.push_back(it->first);
			std::string error_msg = "ERROR :Closing Link: Ping timeout\r\n";
			send(it->first, error_msg.c_str(), error_msg.length(), 0);
			it->second->setQuitReason("Ping timeout");
		} else if (idle_time >= 120 && idle_time < 130) {
			sendToClient(*it->second, "PING :ircserv\r\n");
		}
	}

	for (size_t i = 0; i < to_remove.size(); ++i) {
		_removeClient(to_remove[i]);
	}
}
