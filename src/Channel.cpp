#include "Channel.hpp"
#include "Client.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <sys/socket.h>

Channel::Channel(std::string const &name)
    : _name(name), _topic(), _key(), _user_limit(0),
      _creation_time(std::time(NULL)), _invite_only(false),
      _topic_restricted(false), _members(), _operators(), _invited() {}

Channel::~Channel() {}

// ──────────────────────────── Getters ────────────────────────────

const std::string &Channel::getName() const { return this->_name; }

const std::string &Channel::getTopic() const { return this->_topic; }

const std::string &Channel::getKey() const { return this->_key; }

size_t Channel::getUserLimit() const { return this->_user_limit; }

time_t Channel::getCreationTime() const { return this->_creation_time; }

bool Channel::isInviteOnly() const { return this->_invite_only; }

bool Channel::isTopicRestricted() const { return this->_topic_restricted; }

bool Channel::hasKey() const { return !this->_key.empty(); }

bool Channel::hasUserLimit() const { return this->_user_limit > 0; }

// ──────────────────────────── Setters ────────────────────────────

void Channel::setTopic(const std::string &topic) { this->_topic = topic; }

void Channel::setKey(const std::string &key) { this->_key = key; }

void Channel::removeKey() { this->_key.clear(); }

void Channel::setUserLimit(size_t limit) { this->_user_limit = limit; }

void Channel::removeUserLimit() { this->_user_limit = 0; }

void Channel::setInviteOnly(bool value) { this->_invite_only = value; }

void Channel::setTopicRestricted(bool value) {
  this->_topic_restricted = value;
}

// ──────────────────────────── Member management ─────────────────

void Channel::addMember(Client *client) {
  if (!isMember(client)) {
    this->_members.push_back(client);
  }
}

void Channel::removeMember(Client *client) {
  std::vector<Client *>::iterator it =
      std::find(this->_members.begin(), this->_members.end(), client);
  if (it != this->_members.end()) {
    this->_members.erase(it);
  }
  // Also remove from operators if they were one
  this->_operators.erase(client);
}

bool Channel::isMember(Client *client) const {
  return std::find(this->_members.begin(), this->_members.end(), client) !=
         this->_members.end();
}

bool Channel::isEmpty() const { return this->_members.empty(); }

size_t Channel::getMemberCount() const { return this->_members.size(); }

const std::vector<Client *> &Channel::getMembers() const {
  return this->_members;
}

// ──────────────────────────── Operator management ───────────────

void Channel::addOperator(Client *client) { this->_operators.insert(client); }

void Channel::removeOperator(Client *client) { this->_operators.erase(client); }

bool Channel::isOperator(Client *client) const {
  return this->_operators.find(client) != this->_operators.end();
}

// ──────────────────────────── Invite management ─────────────────

void Channel::addInvited(const std::string &nick) {
  this->_invited.insert(toIrcLower(nick));
}

bool Channel::isInvited(const std::string &nick) const {
  return this->_invited.find(toIrcLower(nick)) != this->_invited.end();
}

void Channel::removeInvited(const std::string &nick) {
  this->_invited.erase(toIrcLower(nick));
}

// ──────────────────────────── Messaging ─────────────────────────

void Channel::broadcastMessage(const std::string &message, Client *exclude) {
  std::string final_msg = message + "\r\n";
  for (size_t i = 0; i < this->_members.size(); ++i) {
    if (this->_members[i] != exclude) {
      this->_members[i]->queueMessage(final_msg);
    }
  }
}

// ──────────────────────────── Utility ───────────────────────────

std::string Channel::getMemberListString() const {
  std::string result;
  for (size_t i = 0; i < this->_members.size(); ++i) {
    if (i > 0)
      result += " ";
    if (isOperator(this->_members[i]))
      result += "@";
    result += this->_members[i]->getNickname();
  }
  return result;
}
