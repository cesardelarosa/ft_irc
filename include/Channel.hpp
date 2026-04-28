#pragma once

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <ctime>
#include <set>
#include <string>
#include <vector>

class Client;

class Channel {
  public:
	Channel(std::string const &name);
	~Channel();

	// Getters
	const std::string &getName() const;
	const std::string &getTopic() const;
	const std::string &getKey() const;
	size_t             getUserLimit() const;
	time_t             getCreationTime() const;
	bool               isInviteOnly() const;
	bool               isTopicRestricted() const;
	bool               hasKey() const;
	bool               hasUserLimit() const;

	// Setters
	void setTopic(const std::string &topic);
	void setKey(const std::string &key);
	void removeKey();
	void setUserLimit(size_t limit);
	void removeUserLimit();
	void setInviteOnly(bool value);
	void setTopicRestricted(bool value);

	// Member management
	void                         addMember(Client *client);
	void                         removeMember(Client *client);
	bool                         isMember(Client *client) const;
	bool                         isEmpty() const;
	size_t                       getMemberCount() const;
	const std::vector<Client *> &getMembers() const;

	// Operator management
	void addOperator(Client *client);
	void removeOperator(Client *client);
	bool isOperator(Client *client) const;

	// Invite management
	void addInvited(const std::string &nick);
	bool isInvited(const std::string &nick) const;
	void removeInvited(const std::string &nick);

	// Messaging
	void broadcastMessage(const std::string &message, Client *exclude);

	// Utility
	std::string getMemberListString() const;

  private:
	std::string _name;
	std::string _topic;
	std::string _key;
	size_t      _user_limit;
	time_t      _creation_time;

	bool _invite_only;
	bool _topic_restricted;

	std::vector<Client *> _members;
	std::set<Client *>    _operators;
	std::set<std::string> _invited;

	Channel();
	Channel(Channel const &src);
	Channel &operator=(Channel const &rhs);
};

#endif
