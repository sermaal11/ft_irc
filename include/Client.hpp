/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 22:36:15 by volmer            #+#    #+#             */
/*   Updated: 2026/02/27 12:26:16 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Utils.hpp"

/*
 * Connected IRC client: stores identity, registration state, and input buffer.
 *
 * _clientFd        : Socket file descriptor
 * _nickname        : IRC nickname (e.g. "sergio")
 * _username        : Username from USER command
 * _hostname        : Client hostname
 * _isAuthenticated : True after correct PASS
 * _hasPassGiven    : True after PASS sent
 * _hasNickGiven    : True after NICK sent
 * _hasUserGiven    : True after USER sent
 * _isRegistered    : True after welcome (001) sent
 * _inputBuffer     : Accumulates raw socket data until full commands (\r\n)
 */
class Client {
private:
  int _clientFd;

  std::string _nickname;
  std::string _username;
  std::string _hostname;

  bool _isAuthenticated;
  bool _hasPassGiven;
  bool _hasNickGiven;
  std::string _inputBuffer;
  std::string _outputBuffer;
  bool _hasUserGiven;
  bool _isRegistered;

public:
  Client(int fd);
  ~Client();

  int getClientFd() const;
  std::string getNickname() const;
  std::string getUsername() const;

  bool getIsAuthenticated() const;
  bool getHasNickGiven() const;
  bool getHasUserGiven() const;
  bool getIsRegistered() const;
  void setIsRegistered(bool val);

  std::string getInputBuffer();

  void setNickname(const std::string nickname);
  void setUsername(const std::string username);

  void setIsAuthenticated(bool isAuthenticated);
  void setHasNickGiven(bool hasNickGiven);
  void setHasUserGiven(const bool hasUserGiven);

  void setInputBuffer(const std::string inputBuffer);
  void clearInputBuffer();

  // Appends raw data to the buffer (data may arrive fragmented)
  void addToBuffer(const std::string input);

  // Returns true if at least one complete command (\n) is in the buffer
  bool hasAllCommand();

  // Extracts and removes the first complete line from the buffer (without \r\n)
  std::string extractCommand();

  // Appends msg to the output buffer (to be flushed when POLLOUT fires)
  void queueOutput(const std::string &msg);

  // Sends as much of the output buffer as possible; returns true when fully emptied
  bool flushOutput();

  // Returns true if there is unsent data in the output buffer
  bool hasPendingOutput() const;

  // Extracts and removes the first whitespace-delimited token from the buffer
  std::string extractToken();
};

#endif