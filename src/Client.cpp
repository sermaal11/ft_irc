/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 22:48:43 by volmer            #+#    #+#             */
/*   Updated: 2026/02/23 21:10:25 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Client.hpp"

/* Initializes a newly connected client; all registration flags start false. */
Client::Client(int fd)
    : _clientFd(fd),
      _nickname(""),
      _username(""),
      _hostname(""),
      _isAuthenticated(false),
      _hasPassGiven(false),
      _hasNickGiven(false),
      _inputBuffer(""),
      _hasUserGiven(false),
      _isRegistered(false)
{
}

/* Destructor. The fd is closed by Server, not here. */
Client::~Client()
{
}

/* Returns true if USER has been sent. */
bool Client::getHasUserGiven() const
{
	return _hasUserGiven;
}

/* Returns true if the welcome (001) has been sent to this client. */
bool Client::getIsRegistered() const
{
    return _isRegistered;
}

/* Marks the client as fully registered. */
void Client::setIsRegistered(bool val)
{
    _isRegistered = val;
}

/* Returns the socket file descriptor. */
int Client::getClientFd() const
{
    return _clientFd;
}

/* Returns the client's current nickname. */
std::string Client::getNickname() const
{
    return _nickname;
}

/* Returns the client's username. */
std::string Client::getUsername() const
{
    return _username;
}

/* Returns true if PASS was sent and matched the server password. */
bool Client::getIsAuthenticated() const
{
    return _isAuthenticated;
}

/* Returns true if NICK has been sent. */
bool Client::getHasNickGiven() const
{
    return _hasNickGiven;
}

/* Returns the raw content of the input buffer. */
std::string Client::getInputBuffer()
{
    return _inputBuffer;
}

/* Sets the client's nickname. */
void Client::setNickname(const std::string nickname)
{
    _nickname = nickname;
}

/* Sets the client's username. */
void Client::setUsername(const std::string username)
{
    _username = username;
}

/* Sets the authentication state (true after correct PASS). */
void Client::setIsAuthenticated(bool isAuthenticated)
{
    _isAuthenticated = isAuthenticated;
}

/* Marks whether NICK has been provided. */
void Client::setHasNickGiven(bool hasNickGiven)
{
    _hasNickGiven = hasNickGiven;
}

/* Replaces the entire input buffer content. */
void Client::setInputBuffer(const std::string inputBuffer)
{
    _inputBuffer = inputBuffer;
}

/* Clears the input buffer. */
void Client::clearInputBuffer()
{
    _inputBuffer.clear();
}

/* Marks whether USER has been provided. */
void Client::setHasUserGiven(const bool hasUserGiven)
{
    _hasUserGiven = hasUserGiven;
}
