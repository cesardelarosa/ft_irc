/**
 * @brief Constructs a new Client object.
 * @param fd The file descriptor of the client's socket.
 */
#include "Client.hpp"

Client::Client(int fd) : _fd(fd), _is_authenticated(false) {
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

/**
 * @brief Sets the nickname for the client.
 * @param nick The new nickname.
 */
void Client::setNickname(const std::string &nick) {
	this->_nickname = nick;
}

/**
 * @brief Gets the nickname of the client.
 * @return A const reference to the nickname string.
 */
const std::string &Client::getNickname() const {
	return this->_nickname;
}

/**
 * @brief Sets the username for the client.
 * @param user The new username.
 */
void Client::setUsername(const std::string &user) {
	this->_username = user;
}

/**
 * @brief Gets the username of the client.
 * @return A const reference to the username string.
 */
const std::string &Client::getUsername() const {
	return this->_username;
}

/**
 * @brief Sets the authentication status of the client.
 * @param value The new authentication status (true or false).
 */
void Client::setAuthenticated(bool value) {
	this->_is_authenticated = value;
}

/**
 * @brief Checks if the client is authenticated.
 * @return True if the client is authenticated, false otherwise.
 */
bool Client::isAuthenticated() const {
	return this->_is_authenticated;
}

/**
 * @brief Gets the file descriptor of the client.
 * @return The client's file descriptor.
 */
int Client::getFd() const {
	return this->_fd;
}