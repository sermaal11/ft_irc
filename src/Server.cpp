/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:48:06 by sergio            #+#    #+#             */
/*   Updated: 2026/02/25 13:23:52 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Channel.hpp"
#include "../include/Utils.hpp"

/* Initializes the server with the given port and password. */
Server::Server(int port, std::string &password)
    : _port(port), _password(password), _serverName("ircserv"), _serverFd(-1) {}

/* Frees all clients and channels, then closes the server socket. */
Server::~Server() {
  std::map<int, Client *>::iterator cit;
  for (cit = _clients.begin(); cit != _clients.end(); ++cit) {
    ::close(cit->first);
    delete cit->second;
  }
  _clients.clear();

  std::map<std::string, Channel *>::iterator chit;
  for (chit = _channels.begin(); chit != _channels.end(); ++chit) {
    delete chit->second;
  }
  _channels.clear();

  if (_serverFd >= 0)
    ::close(_serverFd);
}

/* Sends the IRC welcome sequence (001-004) to a newly registered client. */
void Server::sendWelcomeMessage(Client *client) {
  int fd = client->getClientFd();
  const std::string &nick = client->getNickname();

  std::string msg001 = ":" + _serverName + " 001 " + nick +
                       " :Welcome to the Internet Relay Network " +
                       nick + "!" + client->getUsername() + "@localhost\r\n";
  std::string msg002 = ":" + _serverName + " 002 " + nick +
                       " :Your host is " + _serverName +
                       ", running version 1.0\r\n";
  std::string msg003 = ":" + _serverName + " 003 " + nick +
                       " :This server was created 2026\r\n";
  std::string msg004 = ":" + _serverName + " 004 " + nick + " " +
                       _serverName + " 1.0 o itkol\r\n";

  ::send(fd, msg001.c_str(), msg001.length(), 0);
  ::send(fd, msg002.c_str(), msg002.length(), 0);
  ::send(fd, msg003.c_str(), msg003.length(), 0);
  ::send(fd, msg004.c_str(), msg004.length(), 0);
}

/* Checks if PASS+NICK+USER are complete; sends welcome (001) on first completion. */
void Server::checkClientRegister(Client *client) {
  if (client->getIsAuthenticated() && client->getHasNickGiven() &&
      client->getHasUserGiven() && !client->getIsRegistered()) {
    client->setIsRegistered(true);
    sendWelcomeMessage(client);
  }
}

/*
 * Removes a client: broadcasts QUIT to its channels, frees Client*, removes
 * from _clients and _pollFds, clears bot strikes, and closes the socket.
 */
void Server::removeClient(int fd, const std::string &reason) {
  std::map<int, Client *>::iterator it = _clients.find(fd);
  if (it != _clients.end()) {
    removeClientFromChannels(it->second, reason);
    delete it->second;
    _clients.erase(it);
  }
  _botWarnings.erase(fd);
  for (size_t i = 0; i < _pollFds.size(); i++) {
    if (_pollFds[i].fd == fd) {
      _pollFds.erase(_pollFds.begin() + i);
      break;
    }
  }
  ::close(fd);
  std::cout << YELLOW << "[-] Client disconnected (fd=" << fd << ")" << RESET << "\n";
}

/*
 * Removes a client from all channels it belongs to.
 * Broadcasts QUIT to remaining members and destroys any channel that becomes empty.
 */
void Server::removeClientFromChannels(Client *client, const std::string &reason) {
  int fd = client->getClientFd();
  std::string quitMsg =
      ":" + client->getNickname() + " QUIT :" + reason + "\r\n";

  std::map<std::string, Channel *>::iterator it = _channels.begin();
  while (it != _channels.end()) {
    if (it->second->isMember(fd)) {
      it->second->broadcastMessage(quitMsg, fd);
      it->second->removeMember(fd);

      if (it->second->getMemberCount() == 0) {
        delete it->second;
        _channels.erase(it++);
        continue;
      }
    }
    ++it;
  }
}

/*
 * Handles QUIT: sends ERROR to the client (RFC 2812) then removes it.
 * Format: QUIT [:message]
 */
void Server::handleQuit(Client *client, const std::string &quitMessage) {
  std::string reason = quitMessage.empty() ? "Client Quit" : quitMessage;
  std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
  std::string errorMsg =
      "ERROR :Closing link (" + nick + ") [Quit: " + reason + "]\r\n";
  ::send(client->getClientFd(), errorMsg.c_str(), errorMsg.length(), 0);
  removeClient(client->getClientFd(), reason);
}

/*
 * Handles JOIN: joins or creates a channel, enforcing modes +i, +k, +l.
 * The channel creator becomes operator. Sends JOIN, RPL_TOPIC (332),
 * RPL_NAMREPLY (353), and RPL_ENDOFNAMES (366) on success.
 * Format: JOIN #channel [key]
 */
void Server::handleJoin(Client *client) {
  std::string channelName = client->extractToken();

  if (channelName.empty()) {
    std::string err = ":" + _serverName + " 461 " + client->getNickname() +
                      " JOIN :Not enough parameters\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  if (channelName[0] != '#') {
    std::string err = ":" + _serverName + " 403 " + client->getNickname() + " " +
                      channelName + " :No such channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  bool isNew = false;
  if (_channels.find(channelName) == _channels.end()) {
    Channel *newChan;
    try {
      newChan = new Channel(channelName);
    } catch (std::bad_alloc &) {
      std::string err = ":" + _serverName + " 403 " + client->getNickname() +
                        " " + channelName + " :Cannot create channel\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }
    _channels[channelName] = newChan;
    isNew = true;
  }

  Channel *channel = _channels[channelName];

  if (channel->isMember(client->getClientFd()))
    return;

  if (!isNew && channel->getInviteOnly()) {
    if (!channel->isInvited(client->getNickname())) {
      std::string err = ":" + _serverName + " 473 " + client->getNickname() + " " +
                        channelName + " :Cannot join channel (+i)\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }
  }

  if (!isNew && !channel->getKey().empty()) {
    std::string key = client->extractToken();
    if (key != channel->getKey()) {
      std::string err = ":" + _serverName + " 475 " + client->getNickname() + " " +
                        channelName + " :Cannot join channel (+k)\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }
  }

  if (!isNew && channel->getUserLimit() > 0) {
    if (channel->getMemberCount() >= channel->getUserLimit()) {
      std::string err = ":" + _serverName + " 471 " + client->getNickname() + " " +
                        channelName + " :Cannot join channel (+l)\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }
  }

  channel->addMember(client);
  channel->removeInvite(client->getNickname());

  if (isNew)
    channel->addOperator(client);

  std::string joinMsg =
      ":" + client->getNickname() + " JOIN " + channelName + "\r\n";
  channel->broadcastMessage(joinMsg, -1);

  if (!channel->getTopic().empty()) {
    std::string topicMsg = ":" + _serverName + " 332 " + client->getNickname() + " " +
                           channelName + " :" + channel->getTopic() + "\r\n";
    ::send(client->getClientFd(), topicMsg.c_str(), topicMsg.length(), 0);
  }

  std::string namesMsg = ":" + _serverName + " 353 " + client->getNickname() + " = " +
                         channelName + " :" + channel->getMemberList() + "\r\n";
  ::send(client->getClientFd(), namesMsg.c_str(), namesMsg.length(), 0);

  std::string endMsg = ":" + _serverName + " 366 " + client->getNickname() + " " +
                       channelName + " :End of /NAMES list\r\n";
  ::send(client->getClientFd(), endMsg.c_str(), endMsg.length(), 0);
}

/*
 * Handles PRIVMSG: sends a message to a channel or directly to a user.
 * Channel messages are filtered by the mod bot before delivery.
 * Format: PRIVMSG <target> :<message>
 */
void Server::handlePrivmsg(Client *client, const std::string &params) {
  if (params.empty()) {
    std::string err = ":" + _serverName + " 411 " + client->getNickname() +
                      " :No recipient given (PRIVMSG)\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  size_t spacePos = params.find(' ');
  if (spacePos == std::string::npos) {
    std::string err =
        ":" + _serverName + " 412 " + client->getNickname() + " :No text to send\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  std::string target = params.substr(0, spacePos);
  std::string messageText = params.substr(spacePos + 1);

  if (!messageText.empty() && messageText[0] == ':')
    messageText = messageText.substr(1);

  if (messageText.empty()) {
    std::string err =
        ":" + _serverName + " 412 " + client->getNickname() + " :No text to send\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  std::string fullMsg = ":" + client->getNickname() + " PRIVMSG " + target +
                        " :" + messageText + "\r\n";

  if (target[0] == '#') {
    std::map<std::string, Channel *>::iterator it = _channels.find(target);
    if (it == _channels.end()) {
      std::string err = ":" + _serverName + " 403 " + client->getNickname() + " " + target +
                        " :No such channel\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }
    Channel *channel = it->second;

    if (!channel->isMember(client->getClientFd())) {
      std::string err = ":" + _serverName + " 404 " + client->getNickname() + " " + target +
                        " :Cannot send to channel\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }

    if (botProcessMessage(client, channel, target, messageText))
      return;

    channel->broadcastMessage(fullMsg, client->getClientFd());
  } else {
    Client *targetClient = NULL;
    std::map<int, Client *>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it) {
      if (it->second->getNickname() == target) {
        targetClient = it->second;
        break;
      }
    }

    if (targetClient == NULL) {
      std::string err = ":" + _serverName + " 401 " + client->getNickname() + " " + target +
                        " :No such nick/channel\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }

    ::send(targetClient->getClientFd(), fullMsg.c_str(), fullMsg.length(), 0);
  }
}

/* Returns pointer to client with the given nickname, or NULL if not found. */
Client *Server::findClientByNick(const std::string &nickname) {
  std::map<int, Client *>::iterator it;
  for (it = _clients.begin(); it != _clients.end(); ++it) {
    if (it->second->getNickname() == nickname)
      return it->second;
  }
  return NULL;
}

/*
 * Handles PART: removes client from a channel and notifies all members.
 * Destroys the channel if it becomes empty.
 * Format: PART #channel [:message]
 */
void Server::handlePart(Client *client, const std::string &params) {
  if (params.empty()) {
    std::string err = ":" + _serverName + " 461 " + client->getNickname() +
                      " PART :Not enough parameters\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  size_t spacePos = params.find(' ');
  std::string channelName =
      (spacePos != std::string::npos) ? params.substr(0, spacePos) : params;
  std::string partMsg =
      (spacePos != std::string::npos) ? params.substr(spacePos + 1) : "";
  if (!partMsg.empty() && partMsg[0] == ':')
    partMsg = partMsg.substr(1);

  std::map<std::string, Channel *>::iterator it = _channels.find(channelName);
  if (it == _channels.end()) {
    std::string err = ":" + _serverName + " 403 " + client->getNickname() + " " +
                      channelName + " :No such channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  Channel *channel = it->second;
  if (!channel->isMember(client->getClientFd())) {
    std::string err = ":" + _serverName + " 442 " + client->getNickname() + " " +
                      channelName + " :You're not on that channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  std::string msg = ":" + client->getNickname() + " PART " + channelName;
  if (!partMsg.empty())
    msg += " :" + partMsg;
  msg += "\r\n";
  channel->broadcastMessage(msg, -1);

  channel->removeMember(client->getClientFd());

  if (channel->getMemberCount() == 0) {
    delete channel;
    _channels.erase(it);
  }
}

/*
 * Handles KICK: operator-only expulsion of a user from a channel.
 * Notifies all members before removing the target.
 * Format: KICK #channel nickname [:reason]
 */
void Server::handleKick(Client *client, const std::string &params) {
  if (params.empty()) {
    std::string err = ":" + _serverName + " 461 " + client->getNickname() +
                      " KICK :Not enough parameters\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  std::istringstream iss(params);
  std::string channelName, targetNick, reason;
  iss >> channelName >> targetNick;

  std::string rest;
  if (std::getline(iss, rest) && !rest.empty()) {
    size_t start = rest.find_first_not_of(' ');
    if (start != std::string::npos) {
      reason = rest.substr(start);
      if (!reason.empty() && reason[0] == ':')
        reason = reason.substr(1);
    }
  }
  if (reason.empty())
    reason = client->getNickname();

  if (targetNick.empty()) {
    std::string err = ":" + _serverName + " 461 " + client->getNickname() +
                      " KICK :Not enough parameters\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  std::map<std::string, Channel *>::iterator it = _channels.find(channelName);
  if (it == _channels.end()) {
    std::string err = ":" + _serverName + " 403 " + client->getNickname() + " " +
                      channelName + " :No such channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  Channel *channel = it->second;

  if (!channel->isOperator(client->getClientFd())) {
    std::string err = ":" + _serverName + " 482 " + client->getNickname() + " " +
                      channelName + " :You're not channel operator\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  Client *target = findClientByNick(targetNick);
  if (!target || !channel->isMember(target->getClientFd())) {
    std::string err = ":" + _serverName + " 441 " + client->getNickname() + " " +
                      targetNick + " " + channelName +
                      " :They aren't on that channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  std::string kickMsg = ":" + client->getNickname() + " KICK " + channelName +
                        " " + targetNick + " :" + reason + "\r\n";
  channel->broadcastMessage(kickMsg, -1);

  channel->removeMember(target->getClientFd());

  if (channel->getMemberCount() == 0) {
    delete channel;
    _channels.erase(it);
  }
}

/*
 * Handles INVITE: adds a nickname to the channel invite list.
 * Requires the inviter to be a member; operator-only if channel is +i.
 * Format: INVITE nickname #channel
 */
void Server::handleInvite(Client *client, const std::string &params) {
  if (params.empty()) {
    std::string err = ":" + _serverName + " 461 " + client->getNickname() +
                      " INVITE :Not enough parameters\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  std::istringstream iss(params);
  std::string targetNick, channelName;
  iss >> targetNick >> channelName;

  if (channelName.empty()) {
    std::string err = ":" + _serverName + " 461 " + client->getNickname() +
                      " INVITE :Not enough parameters\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  std::map<std::string, Channel *>::iterator it = _channels.find(channelName);
  if (it == _channels.end()) {
    std::string err = ":" + _serverName + " 403 " + client->getNickname() + " " +
                      channelName + " :No such channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  Channel *channel = it->second;

  if (!channel->isMember(client->getClientFd())) {
    std::string err = ":" + _serverName + " 442 " + client->getNickname() + " " +
                      channelName + " :You're not on that channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  if (channel->getInviteOnly() && !channel->isOperator(client->getClientFd())) {
    std::string err = ":" + _serverName + " 482 " + client->getNickname() + " " +
                      channelName + " :You're not channel operator\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  Client *target = findClientByNick(targetNick);
  if (!target) {
    std::string err = ":" + _serverName + " 401 " + client->getNickname() + " " +
                      targetNick + " :No such nick/channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  if (channel->isMember(target->getClientFd())) {
    std::string err = ":" + _serverName + " 443 " + client->getNickname() + " " +
                      targetNick + " " + channelName +
                      " :is already on channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  channel->addInvite(targetNick);

  std::string confirmMsg = ":" + _serverName + " 341 " + client->getNickname() + " " +
                           targetNick + " " + channelName + "\r\n";
  ::send(client->getClientFd(), confirmMsg.c_str(), confirmMsg.length(), 0);

  std::string inviteMsg = ":" + client->getNickname() + " INVITE " +
                          targetNick + " " + channelName + "\r\n";
  ::send(target->getClientFd(), inviteMsg.c_str(), inviteMsg.length(), 0);
}

/*
 * Handles TOPIC: shows the current topic (no arg) or sets a new one.
 * Requires operator if mode +t is active.
 * Format: TOPIC #channel [:new_topic]
 */
void Server::handleTopic(Client *client, const std::string &params) {
  if (params.empty()) {
    std::string err = ":" + _serverName + " 461 " + client->getNickname() +
                      " TOPIC :Not enough parameters\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  size_t spacePos = params.find(' ');
  std::string channelName =
      (spacePos != std::string::npos) ? params.substr(0, spacePos) : params;

  std::map<std::string, Channel *>::iterator it = _channels.find(channelName);
  if (it == _channels.end()) {
    std::string err = ":" + _serverName + " 403 " + client->getNickname() + " " +
                      channelName + " :No such channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  Channel *channel = it->second;

  if (!channel->isMember(client->getClientFd())) {
    std::string err = ":" + _serverName + " 442 " + client->getNickname() + " " +
                      channelName + " :You're not on that channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  if (spacePos == std::string::npos) {
    if (channel->getTopic().empty()) {
      std::string msg = ":" + _serverName + " 331 " + client->getNickname() + " " +
                        channelName + " :No topic is set\r\n";
      ::send(client->getClientFd(), msg.c_str(), msg.length(), 0);
    } else {
      std::string msg = ":" + _serverName + " 332 " + client->getNickname() + " " +
                        channelName + " :" + channel->getTopic() + "\r\n";
      ::send(client->getClientFd(), msg.c_str(), msg.length(), 0);
    }
    return;
  }

  if (channel->getTopicRestricted() &&
      !channel->isOperator(client->getClientFd())) {
    std::string err = ":" + _serverName + " 482 " + client->getNickname() + " " +
                      channelName + " :You're not channel operator\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  std::string newTopic = params.substr(spacePos + 1);
  if (!newTopic.empty() && newTopic[0] == ':')
    newTopic = newTopic.substr(1);

  channel->setTopic(newTopic);

  std::string topicMsg = ":" + client->getNickname() + " TOPIC " + channelName +
                         " :" + newTopic + "\r\n";
  channel->broadcastMessage(topicMsg, -1);
}

/*
 * Handles MODE: sets or clears channel modes i, t, k, o, l.
 * Without a mode string, replies with RPL_CHANNELMODEIS (324).
 * Requires operator to change modes.
 * Format: MODE #channel [+/-modes [params]]
 */
void Server::handleMode(Client *client, const std::string &params) {
  if (params.empty()) {
    std::string err = ":" + _serverName + " 461 " + client->getNickname() +
                      " MODE :Not enough parameters\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  std::istringstream iss(params);
  std::string channelName, modeStr;
  iss >> channelName >> modeStr;

  if (channelName.empty() || channelName[0] != '#')
    return;

  std::map<std::string, Channel *>::iterator it = _channels.find(channelName);
  if (it == _channels.end()) {
    std::string err = ":" + _serverName + " 403 " + client->getNickname() + " " +
                      channelName + " :No such channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  Channel *channel = it->second;

  if (modeStr.empty()) {
    std::string msg = ":" + _serverName + " 324 " + client->getNickname() + " " +
                      channelName + " " + channel->getModeString() + "\r\n";
    ::send(client->getClientFd(), msg.c_str(), msg.length(), 0);
    return;
  }

  if (!channel->isOperator(client->getClientFd())) {
    std::string err = ":" + _serverName + " 482 " + client->getNickname() + " " +
                      channelName + " :You're not channel operator\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  // Parse modes; track added/removed separately to build the reply string
  bool adding = true;
  std::string addedModes, removedModes, addedParams, removedParams;

  std::vector<std::string> modeParams;
  std::string p;
  while (iss >> p)
    modeParams.push_back(p);
  size_t paramIdx = 0;

  for (size_t i = 0; i < modeStr.length(); i++) {
    char c = modeStr[i];

    if (c == '+') { adding = true;  continue; }
    if (c == '-') { adding = false; continue; }

    switch (c) {
    case 'i':
      channel->setInviteOnly(adding);
      (adding ? addedModes : removedModes) += c;
      break;

    case 't':
      channel->setTopicRestricted(adding);
      (adding ? addedModes : removedModes) += c;
      break;

    case 'k':
      if (adding) {
        if (paramIdx < modeParams.size()) {
          channel->setKey(modeParams[paramIdx]);
          addedModes  += c;
          addedParams += " " + modeParams[paramIdx];
          paramIdx++;
        }
      } else {
        channel->setKey("");
        removedModes += c;
      }
      break;

    case 'o':
      if (paramIdx < modeParams.size()) {
        Client *target = findClientByNick(modeParams[paramIdx]);
        if (target && channel->isMember(target->getClientFd())) {
          if (adding) {
            channel->addOperator(target);
            addedModes    += c;
            addedParams   += " " + modeParams[paramIdx];
          } else {
            channel->removeOperator(target->getClientFd());
            removedModes  += c;
            removedParams += " " + modeParams[paramIdx];
          }
        }
        paramIdx++;
      }
      break;

    case 'l':
      if (adding) {
        if (paramIdx < modeParams.size()) {
          int limit = std::atoi(modeParams[paramIdx].c_str());
          if (limit > 0) {
            channel->setUserLimit(limit);
            addedModes  += c;
            addedParams += " " + modeParams[paramIdx];
          }
          paramIdx++;
        }
      } else {
        channel->setUserLimit(0);
        removedModes += c;
      }
      break;

    default: {
      std::string err = ":" + _serverName + " 472 " + client->getNickname() + " " +
                        std::string(1, c) + " :is unknown mode char to me\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      break;
    }
    }
  }

  std::string appliedModes;
  if (!addedModes.empty())   appliedModes += "+" + addedModes   + addedParams;
  if (!removedModes.empty()) appliedModes += "-" + removedModes + removedParams;
  if (!appliedModes.empty()) {
    std::string modeMsg = ":" + client->getNickname() + " MODE " + channelName +
                          " " + appliedModes + "\r\n";
    channel->broadcastMessage(modeMsg, -1);
  }
}

/*
 * Creates the TCP server socket with SO_REUSEADDR and O_NONBLOCK.
 * Returns the fd on success, or -1 on error.
 */
int Server::createServerSocket() {
  int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd < 0) {
    std::cerr << RED << "socket() failed: " << std::strerror(errno) << RESET << "\n";
    return (-1);
  }

  int opt = 1;
  if (::setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::cerr << RED << "setsockopt(SO_REUSEADDR) failed: " << std::strerror(errno) << RESET << "\n";
    ::close(serverFd);
    return (-1);
  }

  if (fcntl(serverFd, F_SETFL, O_NONBLOCK) < 0) {
    std::cerr << RED << "fcntl(O_NONBLOCK) failed: " << std::strerror(errno) << RESET << "\n";
    ::close(serverFd);
    return (-1);
  }

  return serverFd;
}

/*
 * Binds the server socket to INADDR_ANY:_port and starts listening.
 * Returns false on bind() or listen() failure.
 */
bool Server::bindAndListen() {
  struct sockaddr_in address;
  std::memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(_port);

  if (::bind(_serverFd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    std::cerr << RED << "bind() failed: " << std::strerror(errno) << RESET << "\n";
    return false;
  }

  if (::listen(_serverFd, SOMAXCONN) < 0) {
    std::cerr << RED << "listen() failed: " << std::strerror(errno) << RESET << "\n";
    return false;
  }
  return true;
}

/*
 * Accepts a new client connection, sets it to O_NONBLOCK, and registers it
 * in _clients and _pollFds.
 */
void Server::acceptNewClient() {
  int clientFd = ::accept(_serverFd, NULL, NULL);
  if (clientFd < 0) {
    std::cerr << RED << "accept() failed: " << std::strerror(errno) << RESET << "\n";
    return;
  }

  if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
    std::cerr << RED << "fcntl(O_NONBLOCK) on client failed" << RESET << "\n";
    ::close(clientFd);
    return;
  }

  Client *newClient;
  try {
    newClient = new Client(clientFd);
  } catch (std::bad_alloc &) {
    std::cerr << RED << "Out of memory: cannot allocate new client" << RESET << "\n";
    ::close(clientFd);
    return;
  }
  _clients[clientFd] = newClient;

  pollfd clientPollFd;
  clientPollFd.fd = clientFd;
  clientPollFd.events = POLLIN;
  clientPollFd.revents = 0;
  _pollFds.push_back(clientPollFd);

  std::cout << GREEN << "[+] Client connected (fd=" << clientFd << ")" << RESET << "\n";
}

/*
 * Starts the server: creates socket, binds, then runs the poll() event loop.
 * SIGPIPE is ignored so a broken client socket does not kill the process.
 * POLLHUP/POLLERR are treated as POLLIN and handled by handleClientData().
 */
void Server::run() {
  signal(SIGPIPE, SIG_IGN);

  _serverFd = createServerSocket();
  if (_serverFd < 0)
    return;

  if (!bindAndListen()) {
    ::close(_serverFd);
    _serverFd = -1;
    return;
  }

  std::cout << GREEN << "[SERVER] Listening on port " << _port << RESET << "\n";

  pollfd serverPollFd;
  serverPollFd.fd = _serverFd;
  serverPollFd.events = POLLIN;
  serverPollFd.revents = 0;
  _pollFds.push_back(serverPollFd);

  while (g_running) {
    int pollCount = ::poll(&_pollFds[0], _pollFds.size(), -1);
    if (pollCount < 0) {
      // EINTR means poll() was interrupted by a signal (e.g. SIGINT) — clean exit
      if (errno != EINTR)
        std::cerr << RED << "poll() failed: " << std::strerror(errno) << RESET << "\n";
      break;
    }

    for (size_t i = 0; i < _pollFds.size(); i++) {
      if (_pollFds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
        if (_pollFds[i].fd == _serverFd) {
          acceptNewClient();
        } else {
          size_t sizeBefore = _pollFds.size();
          handleClientData(i);
          // If the client was removed, _pollFds shrank: decrement i so the
          // loop increment doesn't skip the element now at position i.
          if (_pollFds.size() < sizeBefore)
            i--;
        }
      }
    }
  }
}
