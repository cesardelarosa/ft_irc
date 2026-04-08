#pragma once

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Socket.hpp"
#include <string>
#include <sys/types.h>

class Client {
  public:
	Client(int fd);
	~Client();

	void         addToBuffer(const char *data, ssize_t nbytes);
	std::string &getBuffer();

	void               setNickname(const std::string &nick);
	const std::string &getNickname() const;
	void               setUsername(const std::string &user);
	const std::string &getUsername() const;
	void               setRealname(const std::string &name);
	const std::string &getRealname() const;
	void               setHostname(const std::string &host);
	const std::string &getHostname() const;
	int                getFd() const;

	// Registration state
	void setHasPass(bool value);
	bool hasPass() const;
	bool hasNick() const;
	bool hasUser() const;
	void setRegistered(bool value);
	bool isRegistered() const;

	// Prefix for IRC messages
	std::string getPrefix() const;

	// Write buffer (POLLOUT)
	void               queueMessage(const std::string &msg);
	const std::string &getSendBuffer() const;
	void               clearSentBytes(size_t n);
	bool               hasPendingData() const;

	bool               isDisconnected() const;
	void               setDisconnected();
	const std::string &getQuitReason() const;
	void               setQuitReason(const std::string &reason);

  private:
	Socket _socket;

	std::string _buffer;
	std::string _send_buffer;
	std::string _quit_reason;

	std::string _nickname;
	std::string _username;
	std::string _realname;
	std::string _hostname;
	bool        _has_pass;
	bool        _is_registered;
	bool        _disconnected;

	Client();
	Client(Client const &src);
	Client &operator=(Client const &rhs);
};

#endif
