#pragma once

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Socket.hpp"
#include <ctime>
#include <string>
#include <sys/types.h>

class Client {
public:
  Client(int fd);
  ~Client();

  // Core operations
  void addToBuffer(const char *data, ssize_t nbytes);
  void queueMessage(const std::string &msg);
  void clearSentBytes(size_t n);
  void updateActivity();
  void setDisconnected();

  // Getters
  const std::string &getNickname() const;
  const std::string &getUsername() const;
  const std::string &getRealname() const;
  const std::string &getHostname() const;
  const std::string &getQuitReason() const;
  const std::string &getSendBuffer() const;
  std::string &getBuffer();
  std::string getPrefix() const;
  int getFd() const;
  time_t getLastActivity() const;

  // Setters
  void setNickname(const std::string &nick);
  void setUsername(const std::string &user);
  void setRealname(const std::string &name);
  void setHostname(const std::string &host);
  void setQuitReason(const std::string &reason);
  void setHasPass(bool value);
  void setRegistered(bool value);

  // State Checkers
  bool hasPass() const;
  bool hasNick() const;
  bool hasUser() const;
  bool isRegistered() const;
  bool hasPendingData() const;
  bool isDisconnected() const;

private:
  Socket _socket;

  std::string _buffer;
  std::string _send_buffer;
  std::string _quit_reason;

  std::string _nickname;
  std::string _username;
  std::string _realname;
  std::string _hostname;
  bool _has_pass;
  bool _is_registered;
  bool _disconnected;
  time_t _last_activity;

  Client();
  Client(Client const &src);
  Client &operator=(Client const &rhs);
};

#endif
