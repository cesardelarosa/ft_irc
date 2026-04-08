#include "Client.hpp"

/**
 * @brief Constructs a new Client object.
 * @param fd The file descriptor of the client's socket.
 */
Client::Client(int fd)
    : _socket(fd), _quit_reason("Client disconnected"), _has_pass(false),
      _is_registered(false), _disconnected(false) {
	this->_socket.setNonBlocking();
}

/**
 * @brief Destroys the Client object.
 */
Client::~Client() {
}

/**
 * @brief Appends data to the client's internal buffer.
 * @param data A pointer to the data to be appended.
 * @param nbytes The number of bytes to append.
 */
void Client::addToBuffer(const char *data, int nbytes) {
	this->_buffer.append(data, nbytes);
}

/**
 * @brief Gets a reference to the client's internal buffer.
 * @return A reference to the buffer string.
 */
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

void Client::setRealname(const std::string &name) {
	this->_realname = name;
}

const std::string &Client::getRealname() const {
	return this->_realname;
}

void Client::setHostname(const std::string &host) {
	this->_hostname = host;
}

const std::string &Client::getHostname() const {
	return this->_hostname;
}

int Client::getFd() const {
	return this->_socket.get();
}

void Client::setHasPass(bool value) {
	this->_has_pass = value;
}

bool Client::hasPass() const {
	return this->_has_pass;
}

bool Client::hasNick() const {
	return !this->_nickname.empty();
}

bool Client::hasUser() const {
	return !this->_username.empty();
}

void Client::setRegistered(bool value) {
	this->_is_registered = value;
}

bool Client::isRegistered() const {
	return this->_is_registered;
}

/**
 * @brief Returns the IRC prefix string for this client.
 * @details Format: nick!user@host
 */
std::string Client::getPrefix() const {
	std::string host = this->_hostname.empty() ? "localhost" : this->_hostname;
	return this->_nickname + "!" + this->_username + "@" + host;
}

// ──────────────────────────── Write buffer ───────────────────────

void Client::queueMessage(const std::string &msg) {
	this->_send_buffer += msg;
}

const std::string &Client::getSendBuffer() const {
	return this->_send_buffer;
}

void Client::clearSentBytes(size_t n) {
	this->_send_buffer.erase(0, n);
}

bool Client::hasPendingData() const {
	return !this->_send_buffer.empty();
}

bool Client::isDisconnected() const {
	return this->_disconnected;
}

void Client::setDisconnected() {
	this->_disconnected = true;
}

const std::string &Client::getQuitReason() const {
	return this->_quit_reason;
}

void Client::setQuitReason(const std::string &reason) {
	this->_quit_reason = reason;
}
