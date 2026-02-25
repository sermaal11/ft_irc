/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parsing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 14:10:39 by volmer            #+#    #+#             */
/*   Updated: 2026/02/23 21:05:02 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"

/*
 * Routes a parsed IRC command to its handler.
 * Pre-registration commands (PASS, NICK, USER, PING, QUIT) are always allowed.
 * Any other command sent before registration gets ERR_NOTREGISTERED (451).
 */
void Server::proccesCommand(Client *client, std::string command) {
  bool preAuthCmd = (command == "PASS" || command == "NICK" ||
                     command == "USER" || command == "PING" ||
                     command == "QUIT");
  if (!preAuthCmd && !client->getIsRegistered()) {
    std::string err = ":" + _serverName + " 451 * :You have not registered\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  /*
   * NICK: sets or changes the client's nickname.
   * Validates format (RFC 1459: max 9 chars, starts with letter) and
   * uniqueness. Broadcasts nick change to all shared channels if already
   * registered.
   */
  if (command == "NICK") {
    std::string nickname = client->extractToken();
    if (nickname.empty()) {
      std::string err = ":" + _serverName + " 431 * :No nickname given\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    } else {
      bool valid = nickname.length() <= 9 && std::isalpha(nickname[0]);
      for (size_t i = 1; valid && i < nickname.length(); ++i) {
        char c = nickname[i];
        valid = std::isalnum(c) || c == '-' || c == '[' || c == ']' ||
                c == '\\' || c == '\'' || c == '`' || c == '^' ||
                c == '{' || c == '}';
      }
      if (!valid) {
        std::string err = ":" + _serverName + " 432 * " + nickname +
                          " :Erroneous nickname\r\n";
        ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      } else {
        Client *existing = findClientByNick(nickname);
        if (existing != NULL && existing->getClientFd() != client->getClientFd()) {
          std::string err = ":" + _serverName + " 433 * " + nickname +
                            " :Nickname is already in use\r\n";
          ::send(client->getClientFd(), err.c_str(), err.length(), 0);
        } else {
          if (client->getIsRegistered()) {
            std::string oldNick = client->getNickname();
            std::string nickMsg = ":" + oldNick + " NICK " + nickname + "\r\n";
            bool notified = false;
            std::map<std::string, Channel *>::iterator chit;
            for (chit = _channels.begin(); chit != _channels.end(); ++chit) {
              if (chit->second->isMember(client->getClientFd())) {
                chit->second->broadcastMessage(nickMsg, -1);
                notified = true;
              }
            }
            if (!notified)
              ::send(client->getClientFd(), nickMsg.c_str(), nickMsg.length(), 0);
          }
          client->setNickname(nickname);
          client->setHasNickGiven(true);
          checkClientRegister(client);
        }
      }
    }
  }
  /*
   * USER: sets the client's username. Only valid during registration.
   * Format: USER <username> <hostname> <servername> :<realname>
   */
  else if (command == "USER") {
    if (client->getIsRegistered()) {
      std::string err = ":" + _serverName + " 462 " + client->getNickname() +
                        " :You may not reregister\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }
    std::string username = client->extractToken();
    client->setUsername(username);
    client->setHasUserGiven(true);
    checkClientRegister(client);
  }
  /*
   * PASS: authenticates the client against the server password.
   * Must be sent before NICK and USER. Wrong password sends ERR_PASSWDMISMATCH (464).
   */
  else if (command == "PASS") {
    std::string password = client->extractToken();
    if (password == _password) {
      client->setIsAuthenticated(true);
      checkClientRegister(client);
    } else {
      std::string err = ":" + _serverName + " 464 * :Password incorrect\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    }
  }
  /*
   * PING: keep-alive check. Responds with PONG :<token>.
   * Accepts both "PING token" and "PING :token" formats.
   */
  else if (command == "PING") {
    std::string token = client->getInputBuffer();
    size_t start = token.find_first_not_of(" \t\r\n");

    if (start == std::string::npos) {
      std::string pongReply = "PONG\r\n";
      ::send(client->getClientFd(), pongReply.c_str(), pongReply.length(), 0);
    } else {
      size_t end = token.find_first_of("\r\n", start);
      if (end != std::string::npos)
        token = token.substr(start, end - start);
      else
        token = token.substr(start);
      size_t lastNonSpace = token.find_last_not_of(" \t");
      if (lastNonSpace != std::string::npos)
        token = token.substr(0, lastNonSpace + 1);
      std::string pongReply;
      if (!token.empty() && token[0] == ':')
        pongReply = "PONG " + token + "\r\n";
      else
        pongReply = "PONG :" + token + "\r\n";
      ::send(client->getClientFd(), pongReply.c_str(), pongReply.length(), 0);
    }
  }
  /* PRIVMSG: sends a message to a channel or user. */
  else if (command == "PRIVMSG") {
    std::string params = client->getInputBuffer();
    handlePrivmsg(client, params);
  }
  /* JOIN: adds the client to a channel (creates it if new). */
  else if (command == "JOIN") {
    handleJoin(client);
  } else if (command == "PART") {
    std::string params = client->getInputBuffer();
    handlePart(client, params);
  } else if (command == "KICK") {
    std::string params = client->getInputBuffer();
    handleKick(client, params);
  } else if (command == "INVITE") {
    std::string params = client->getInputBuffer();
    handleInvite(client, params);
  } else if (command == "TOPIC") {
    std::string params = client->getInputBuffer();
    handleTopic(client, params);
  } else if (command == "MODE") {
    std::string params = client->getInputBuffer();
    handleMode(client, params);
  } else if (command == "QUIT") {
    std::string quitMsg = client->getInputBuffer();
    size_t start = quitMsg.find_first_not_of(" \t");
    if (start != std::string::npos) {
      quitMsg = quitMsg.substr(start);
      if (!quitMsg.empty() && quitMsg[0] == ':')
        quitMsg = quitMsg.substr(1);
    } else {
      quitMsg = "";
    }
    handleQuit(client, quitMsg);
    return;
  } else {
    std::string err = ":" + _serverName + " 421 " + client->getNickname() +
                      " " + command + " :Unknown command\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
  }
}

/*
 * Reads data from client at _pollFds[i] and dispatches complete commands.
 * Uses recv() (max 512 bytes per IRC spec). On recv() <= 0, removes the client.
 * Data is accumulated in the client buffer; each \n-terminated line is parsed
 * as one command and forwarded to proccesCommand(). Handles use-after-free by
 * checking if the client still exists after each command.
 */
void Server::handleClientData(int i) {
  char buffer[512];
  int clientFd = _pollFds[i].fd;
  int bytesRead = ::recv(_pollFds[i].fd, buffer, sizeof(buffer) - 1, 0);

  if (bytesRead <= 0) {
    if (bytesRead != 0)
      std::cerr << RED << "recv() failed: " << std::strerror(errno) << RESET << "\n";
    removeClient(clientFd);
    return;
  }
  Client *client = _clients[clientFd];
  buffer[bytesRead] = '\0';
  client->addToBuffer(buffer);
  while (client->hasAllCommand()) {
    std::string line = client->extractCommand();

    if (line.empty())
      continue;

    std::istringstream iss(line);
    std::string command;
    iss >> command;

    if (command.empty())
      continue;

    std::string restOfLine;
    if (iss.tellg() != -1) {
      size_t pos = static_cast<size_t>(iss.tellg());
      while (pos < line.length() && line[pos] == ' ')
        pos++;
      restOfLine = line.substr(pos);
    }

    if (!restOfLine.empty())
      client->setInputBuffer(restOfLine);
    else
      client->clearInputBuffer();

    proccesCommand(client, command);

    // Guard against use-after-free if the client was removed during processing
    if (_clients.find(clientFd) == _clients.end())
      return;

    client->clearInputBuffer();
  }
}
