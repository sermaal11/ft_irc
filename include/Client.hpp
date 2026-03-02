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

  void addToBuffer(const std::string input);

  bool hasAllCommand();

  std::string extractCommand();

  void queueOutput(const std::string &msg);

  bool flushOutput();

  bool hasPendingOutput() const;

  std::string extractToken();
};

#endif
