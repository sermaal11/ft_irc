/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/23 10:24:00 by sergio            #+#    #+#             */
/*   Updated: 2026/02/23 10:31:16 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Channel.hpp"
#include "../include/Client.hpp"

/* Initializes channel with the given name; all modes off by default. */
Channel::Channel(std::string name)
    : _name(name), _topic(""), _inviteOnly(false), _topicRestricted(false),
      _key(""), _userLimit(0) {
}

/* Destructor. */
Channel::~Channel() {
}

/* Returns the channel name. */
std::string Channel::getName() const { return _name; }

/* Returns the current topic. */
std::string Channel::getTopic() const { return _topic; }

/* Returns the number of members in the channel. */
int Channel::getMemberCount() const { return _members.size(); }

/* Sets the channel topic. */
void Channel::setTopic(const std::string topic) { _topic = topic; }

/* Adds client as a member. Overwrites silently on duplicate fd. */
void Channel::addMember(Client *client) {
  _members[client->getClientFd()] = client;
}

/* Removes member and any operator entry for the given fd. */
void Channel::removeMember(int fd) {
  _members.erase(fd);
  _operators.erase(fd);
}

/* Returns true if fd belongs to a channel member. */
bool Channel::isMember(int fd) const {
  return _members.find(fd) != _members.end();
}

/* Promotes client to operator. Client must already be a member. */
void Channel::addOperator(Client *client) {
  _operators[client->getClientFd()] = client;
}

/* Removes operator status for the given fd. */
void Channel::removeOperator(int fd) { _operators.erase(fd); }

/* Returns true if fd belongs to a channel operator. */
bool Channel::isOperator(int fd) const {
  return _operators.find(fd) != _operators.end();
}

/* Sends message to all members. excludeFd skips that sender (-1 = send to all). */
void Channel::broadcastMessage(const std::string message, int excludeFd) {
  std::map<int, Client *>::iterator it;
  for (it = _members.begin(); it != _members.end(); ++it) {
    if (it->first != excludeFd) {
      ::send(it->first, message.c_str(), message.length(), 0);
    }
  }
}

/* Returns space-separated nick list; operators prefixed with '@'. Used for RPL_NAMREPLY (353). */
std::string Channel::getMemberList() const {
  std::string list;
  std::map<int, Client *>::const_iterator it;
  for (it = _members.begin(); it != _members.end(); ++it) {
    if (!list.empty())
      list += " ";
    if (isOperator(it->first))
      list += "@";
    list += it->second->getNickname();
  }
  return list;
}

/* Returns the invite-only flag (mode +i). */
bool Channel::getInviteOnly() const { return _inviteOnly; }

/* Sets the invite-only flag (mode +i). */
void Channel::setInviteOnly(bool inviteOnly) { _inviteOnly = inviteOnly; }

/* Returns the topic-restricted flag (mode +t). */
bool Channel::getTopicRestricted() const { return _topicRestricted; }

/* Sets the topic-restricted flag (mode +t). */
void Channel::setTopicRestricted(bool topicRestricted) {
  _topicRestricted = topicRestricted;
}

/* Returns the channel key (mode +k). */
std::string Channel::getKey() const { return _key; }

/* Sets the channel key (mode +k). */
void Channel::setKey(const std::string key) { _key = key; }

/* Returns the user limit (mode +l); 0 means no limit. */
int Channel::getUserLimit() const { return _userLimit; }

/* Sets the user limit (mode +l). */
void Channel::setUserLimit(int limit) { _userLimit = limit; }

/* Adds nickname to the invite list; ignores duplicates. */
void Channel::addInvite(const std::string nickname) {
  if (!isInvited(nickname))
    _inviteList.push_back(nickname);
}

/* Returns true if nickname is in the invite list. */
bool Channel::isInvited(const std::string nickname) const {
  for (size_t i = 0; i < _inviteList.size(); i++) {
    if (_inviteList[i] == nickname)
      return true;
  }
  return false;
}

/* Removes nickname from the invite list. */
void Channel::removeInvite(const std::string nickname) {
  for (std::vector<std::string>::iterator it = _inviteList.begin();
       it != _inviteList.end(); ++it) {
    if (*it == nickname) {
      _inviteList.erase(it);
      return;
    }
  }
}

/* Returns active mode string with parameters (e.g. "+itk key" or "+tl 10"). */
std::string Channel::getModeString() const {
  std::string modes = "+";
  std::string params;

  if (_inviteOnly)
    modes += "i";
  if (_topicRestricted)
    modes += "t";
  if (!_key.empty()) {
    modes += "k";
    params += " " + _key;
  }
  if (_userLimit > 0) {
    modes += "l";
    std::ostringstream oss;
    oss << _userLimit;
    params += " " + oss.str();
  }

  if (modes == "+")
    return "+";
  return modes + params;
}
