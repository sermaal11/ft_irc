/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:43:31 by sergio            #+#    #+#             */
/*   Updated: 2026/02/27 12:28:01 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "Channel.hpp"
#include "Client.hpp"
#include "Utils.hpp"

class Server {
private:
  int _port;
  std::string _password;
  std::string _serverName;

  int _serverFd;
  std::vector<pollfd> _pollFds;

  std::map<int, Client *> _clients;
  std::map<std::string, Channel *> _channels;
  std::map<int, int> _botWarnings;

  int createServerSocket();

  bool bindAndListen();

  void acceptNewClient();

  void handleClientData(int i);

  void removeClient(int fd, const std::string &reason = "Client disconnected");

  void proccesCommand(Client *client, std::string command);

  void handleJoin(Client *client);

  void handlePrivmsg(Client *client, const std::string &params);

  void handlePart(Client *client, const std::string &params);

  void handleKick(Client *client, const std::string &params);

  void handleInvite(Client *client, const std::string &params);

  void handleTopic(Client *client, const std::string &params);

  void handleMode(Client *client, const std::string &params);

  void handleQuit(Client *client, const std::string &quitMessage);

  Client *findClientByNick(const std::string &nickname);

  bool botCheckBadWords(const std::string &message);

  bool botProcessMessage(Client *client, Channel *channel,
                         const std::string &target, const std::string &message);

  void removeClientFromChannels(Client *client, const std::string &reason = "Client disconnected");

  void checkClientRegister(Client *client);

  void sendWelcomeMessage(Client *client);

  void sendMsg(int fd, const std::string &msg);

  void setPollOut(int fd);

  void enablePollOutForPendingClients();

public:
  Server(int port, std::string &password);
  ~Server();

  void run();
};

#endif
