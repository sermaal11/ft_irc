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

/*
 * IRC server: manages multiple clients via TCP sockets and poll().
 *
 * _port        : Listening port (typically 6667)
 * _password    : Required password for PASS command
 * _serverName  : Server name used in IRC responses
 * _serverFd    : Main server socket fd
 * _pollFds     : Monitored fds (server + all clients)
 * _clients     : fd -> Client* map
 * _channels    : name -> Channel* map
 * _botWarnings : fd -> strike count for the mod bot
 */
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

  // Creates TCP socket with SO_REUSEADDR and O_NONBLOCK; returns fd or -1
  int createServerSocket();

  // Binds socket to INADDR_ANY:_port and starts listening; returns false on error
  bool bindAndListen();

  // Accepts a new client connection and registers it in _clients and _pollFds
  void acceptNewClient();

  // Reads data from client at _pollFds[i]; buffers and dispatches complete commands
  void handleClientData(int i);

  // Closes client connection and frees all associated resources
  void removeClient(int fd, const std::string &reason = "Client disconnected");

  // Routes a parsed IRC command to the appropriate handler
  void proccesCommand(Client *client, std::string command);

  // JOIN: adds client to channel, creates it if new (client becomes operator)
  void handleJoin(Client *client);

  // PRIVMSG: sends message to a channel or directly to a user
  void handlePrivmsg(Client *client, const std::string &params);

  // PART: removes client from a channel and notifies remaining members
  void handlePart(Client *client, const std::string &params);

  // KICK: operator-only expulsion of a user from a channel
  void handleKick(Client *client, const std::string &params);

  // INVITE: adds a nick to the channel invite list (operator-only if +i)
  void handleInvite(Client *client, const std::string &params);

  // TOPIC: shows or changes the channel topic (operator-only if +t)
  void handleTopic(Client *client, const std::string &params);

  // MODE: sets or clears channel modes i, t, k, o, l
  void handleMode(Client *client, const std::string &params);

  // QUIT: sends ERROR to client then removes it cleanly (RFC 2812)
  void handleQuit(Client *client, const std::string &quitMessage);

  // Returns client pointer by nickname, or NULL if not found
  Client *findClientByNick(const std::string &nickname);

  // Returns true if message contains a banned word (case-insensitive)
  bool botCheckBadWords(const std::string &message);

  // Applies bot strike logic; returns true if message was blocked
  bool botProcessMessage(Client *client, Channel *channel,
                         const std::string &target, const std::string &message);

  // Removes client from all channels, notifying members and deleting empty channels
  void removeClientFromChannels(Client *client, const std::string &reason = "Client disconnected");

  // Checks if PASS+NICK+USER are complete; sends welcome (001) if so
  void checkClientRegister(Client *client);

  // Sends IRC welcome sequence (001-004) to a newly registered client
  void sendWelcomeMessage(Client *client);

  // Queues msg in the client's output buffer and enables POLLOUT for that fd
  void sendMsg(int fd, const std::string &msg);

  // Enables POLLOUT on the given fd's entry in _pollFds
  void setPollOut(int fd);

  // Enables POLLOUT for every client that has data in its output buffer
  void enablePollOutForPendingClients();

public:
  Server(int port, std::string &password);
  ~Server();

  // Starts the server: creates socket, binds, then enters the poll() event loop
  void run();
};

#endif