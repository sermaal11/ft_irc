/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/23 10:24:00 by sergio            #+#    #+#             */
/*   Updated: 2026/02/23 10:30:51 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "Utils.hpp"

class Client;

class Channel {
private:
  std::string _name;
  std::string _topic;

  bool _inviteOnly;
  bool _topicRestricted;
  std::string _key;
  int _userLimit;

  std::map<int, Client *> _members;
  std::map<int, Client *> _operators;
  std::vector<std::string> _inviteList;

public:
  Channel(std::string name);
  ~Channel();

  std::string getName() const;
  std::string getTopic() const;
  int getMemberCount() const;

  void setTopic(const std::string topic);

  void addMember(Client *client);
  void removeMember(int fd);
  bool isMember(int fd) const;

  void addOperator(Client *client);
  void removeOperator(int fd);
  bool isOperator(int fd) const;

  void broadcastMessage(const std::string message, int excludeFd);

  std::string getMemberList() const;

  bool getInviteOnly() const;
  void setInviteOnly(bool inviteOnly);
  bool getTopicRestricted() const;
  void setTopicRestricted(bool topicRestricted);
  std::string getKey() const;
  void setKey(const std::string key);
  int getUserLimit() const;
  void setUserLimit(int limit);

  void addInvite(const std::string nickname);
  bool isInvited(const std::string nickname) const;
  void removeInvite(const std::string nickname);

  std::string getModeString() const;
};

#endif
