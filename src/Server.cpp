#include "Server.hpp"
#include "Constants.hpp"
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

Server::Server(int port, std::string password)
    : _port(port), _password(password), _clients(), _channels(),
      _serverSocket(), _eventManager(), _commandHandler(this) {
  std::cout << LOG_SERVER << "Server created on port: " << ANSI_BOLD
            << this->_port << LOG_R << std::endl;
}

Server::~Server() {
  for (std::map<int, Client *>::iterator it = this->_clients.begin();
       it != this->_clients.end(); ++it) {
    delete it->second;
  }
  for (std::map<std::string, Channel *>::iterator it = this->_channels.begin();
       it != this->_channels.end(); ++it) {
    delete it->second;
  }
}

void Server::start() {
  _setupServerSocket();
  _runEventLoop();
}
void Server::sendReply(Client &client, const std::string &message) {
  std::string final_message =
      ":" + IRC::Identity::SERVER_NAME + " " + message + "\r\n";
  client.queueMessage(final_message);
}
void Server::sendToClient(Client &client, const std::string &message) {
  client.queueMessage(message);
}

// ──────────────────────────── Accessors ─────────────────────────

const std::string &Server::getPassword() const { return this->_password; }

std::map<int, Client *> &Server::getClients() { return this->_clients; }

std::map<std::string, Channel *> &Server::getChannels() {
  return this->_channels;
}

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

Channel *Server::getChannel(const std::string &name) {
  std::map<std::string, Channel *>::iterator it =
      this->_channels.find(toIrcLower(name));
  if (it != this->_channels.end()) {
    return it->second;
  }
  return NULL;
}

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

void Server::removeChannel(const std::string &name) {
  std::map<std::string, Channel *>::iterator it =
      this->_channels.find(toIrcLower(name));
  if (it != this->_channels.end()) {
    delete it->second;
    this->_channels.erase(it);
  }
}

void Server::removeClientFromAllChannels(Client *client,
                                         const std::string &reason) {
  std::vector<std::string> empty_channels;

  for (std::map<std::string, Channel *>::iterator it = this->_channels.begin();
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

void Server::_setupServerSocket() {
  this->_serverSocket.initServer(this->_port);
  this->_eventManager.addSocket(this->_serverSocket.get(), POLLIN);
}

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
    // 1. Update POLLOUT flags for clients that have pending outbound data
    _updatePollEvents();

    // 2. Block until an event fires or the poll timeout expires
    if (this->_eventManager.waitEvents(IRC::Timing::POLL_TIMEOUT_MS) == -1) {
      // EINTR: interrupted by a signal (e.g. SIGINT), exit gracefully
      if (errno == EINTR)
        break;
      throw std::runtime_error("poll() failed.");
    }

    // 3. Periodically check for idle clients (ping or disconnect)
    time_t now = std::time(NULL);
    if (now - last_timeout_check >= IRC::Timing::TIMEOUT_CHECK_INTERVAL) {
      _checkTimeouts();
      last_timeout_check = now;
    }

    // 4. Index 0 is the server socket: accept new connections
    if (this->_eventManager.getRevents(0) & POLLIN)
      _handleNewConnection();

    // 5. Walk client sockets: handle errors, incoming data, and outgoing data
    for (size_t i = 1; i < this->_eventManager.getSocketCount();) {
      int fd = this->_eventManager.getFd(i);
      short revents = this->_eventManager.getRevents(i);
      bool removed = false;

      // Socket error or peer hangup: remove client immediately
      if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
        std::cerr << LOG_ERROR << "Socket error on fd " << LOG_FD << fd << LOG_R
                  << std::endl;
        _removeClient(fd);
        removed = true;
      } else if (revents & POLLIN) {
        // Incoming data: read, parse commands, possibly disconnect
        if (_handleClientData(fd)) {
          removed = true;
        }
      }

      // Writable and still connected: flush outbound buffer
      if (!removed && (revents & POLLOUT)) {
        if (_handleClientWrite(fd)) {
          removed = true;
        }
      }

      // Only advance index if the fd was not removed (swap-and-pop in remove)
      if (!removed) {
        ++i;
      }
    }
  }

  std::cout << std::endl;
  std::cout << LOG_SERVER << ANSI_BOLD ANSI_YELLOW
            << "Server shutting down gracefully." << LOG_R << std::endl;
}

void Server::_handleNewConnection() {
  std::string ip_str;
  int client_fd = this->_serverSocket.acceptClient(ip_str);

  // -2 means the connection came from a non-IPv4 address (unsupported)
  if (client_fd == -2) {
    std::cerr << LOG_WARN << "Connection rejected: Only IPv4 is supported."
              << std::endl;
    return;
  }

  if (client_fd == -1) {
    std::cerr << LOG_WARN << "accept() failed." << std::endl;
    return;
  }

  // Allocate and register the new client in the event manager
  Client *new_client = NULL;
  try {
    new_client = new Client(client_fd);
    new_client->setHostname(ip_str);
    this->_eventManager.addSocket(client_fd, POLLIN);
    this->_clients.insert(std::make_pair(client_fd, new_client));
  } catch (const std::exception &) {
    std::cerr << LOG_ERROR << "Out of memory for new client" << std::endl;
    // new succeeded but insert/addSocket failed: clean up the object
    if (new_client)
      delete new_client;
    // new itself failed: close the raw fd since no Client owns it
    else
      close(client_fd);
    return;
  }

  std::cout << LOG_CONNECT << ANSI_GREEN << "New connection" << LOG_R
            << " from " << ANSI_BOLD << ip_str << LOG_R << LOG_FD << " (fd "
            << client_fd << ")" << LOG_R << std::endl;
}

bool Server::_handleClientData(int client_fd) {
  char buffer[512];
  ssize_t nbytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

  // 1. Handle recv errors
  if (nbytes < 0) {
    // EAGAIN is normal for non-blocking sockets: no data available yet
    if (errno == EAGAIN)
      return false;
    std::cerr << LOG_ERROR << "recv() failed for fd " << LOG_FD << client_fd
              << LOG_R << std::endl;
    _removeClient(client_fd);
    return true;
  }

  // 2. Zero bytes means the remote end closed the connection
  if (nbytes == 0) {
    std::cout << LOG_DISCONNECT << ANSI_RED << "Client disconnected" << LOG_R
              << LOG_FD << " (fd " << client_fd << ")" << LOG_R << std::endl;
    _removeClient(client_fd);
    return true;
  }

  // 3. Append received data to the client's buffer and process commands
  buffer[nbytes] = '\0';
  std::map<int, Client *>::iterator it = this->_clients.find(client_fd);
  if (it != this->_clients.end()) {
    it->second->updateActivity();
    it->second->addToBuffer(buffer, nbytes);

    // Guard against buffer flooding: disconnect if the buffer grows too large
    if (it->second->getBuffer().size() > IRC::Limits::RECV_BUFFER_MAX) {
      std::cerr << LOG_WARN << "Buffer overflow from fd " << LOG_FD << client_fd
                << LOG_R << ANSI_YELLOW << " — disconnecting" << LOG_R
                << std::endl;
      _removeClient(client_fd);
      return true;
    }
    _processClientCommands(*it->second);

    // 4. If the command processing triggered a QUIT, flush remaining
    //    outbound data and remove the client
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

void Server::_processClientCommands(Client &client) {
  std::string &buffer = client.getBuffer();
  size_t pos = 0;

  while ((pos = buffer.find("\r\n")) != std::string::npos) {
    std::string command_line = buffer.substr(0, pos);
    buffer.erase(0, pos + 2);

    // Truncate oversized lines to prevent abuse
    if (command_line.length() > IRC::Limits::MAX_IRC_LINE) {
      command_line = command_line.substr(0, IRC::Limits::MAX_IRC_LINE);
    }

    if (!command_line.empty()) {
      std::cout << LOG_CMD << LOG_FD << "fd " << client.getFd() << LOG_R << " "
                << ANSI_BRIGHT_WHITE << command_line << LOG_R << std::endl;
      this->_commandHandler.handleCommand(client, command_line);
    }

    // Stop processing if the client disconnected mid-batch (e.g. QUIT)
    if (client.isDisconnected()) {
      break;
    }
  }
}

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

bool Server::_handleClientWrite(int client_fd) {
  std::map<int, Client *>::iterator it = this->_clients.find(client_fd);
  if (it == this->_clients.end() || !it->second->hasPendingData())
    return false;

  const std::string &buf = it->second->getSendBuffer();
  ssize_t sent = send(client_fd, buf.c_str(), buf.length(), 0);

  if (sent < 0) {
    // EAGAIN: socket buffer full, retry on next poll cycle
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

void Server::_checkTimeouts() {
  time_t now = std::time(NULL);
  std::vector<int> to_remove;

  for (std::map<int, Client *>::iterator it = this->_clients.begin();
       it != this->_clients.end(); ++it) {
    time_t idle_time = now - it->second->getLastActivity();

    // Fully timed out: send ERROR and schedule for removal
    if (idle_time > IRC::Timing::TIMEOUT_DISCONNECT) {
      to_remove.push_back(it->first);
      std::string error_msg = "ERROR :Closing Link: Ping timeout\r\n";
      send(it->first, error_msg.c_str(), error_msg.length(), 0);
      it->second->setQuitReason("Ping timeout");

      // Idle but not yet timed out: send a PING to probe liveness
    } else if (idle_time >= IRC::Timing::PING_IDLE_START &&
               idle_time < IRC::Timing::PING_IDLE_END) {
      sendToClient(*it->second, "PING :" + IRC::Identity::SERVER_NAME + "\r\n");
    }
  }

  for (size_t i = 0; i < to_remove.size(); ++i) {
    _removeClient(to_remove[i]);
  }
}
