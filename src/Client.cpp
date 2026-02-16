/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 22:48:43 by volmer            #+#    #+#             */
/*   Updated: 2026/02/16 23:18:52 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Client.hpp"

//======PARSING========
bool Client::hasAllCommand()
{
    return _inputBuffer.find("\r\n") != std::string::npos;
}

std::string Client::extractCommand()
{
    size_t pos = _inputBuffer.find("\r\n");
    if (pos != std::string::npos)
    {
        return "";
	}
	
	std::string command = _inputBuffer.substr(0, pos);
	_inputBuffer.erase(0, pos + 2);
	return command;
}

// Constructor: inicializa el cliente recién conectado
Client::Client(int fd) 
    : _clientFd(fd),
      _nickname(""),           // Sin nickname aún
      _username(""),           // Sin username aún
      _hostname(""),           // Sin hostname aún
      _isAuthenticated(false), // NO está autenticado
      _hasPassGiven(false),    // NO ha dado password
      _hasNickGiven(false),    // NO tiene nickname
      _inputBuffer("")         // Buffer vacío
{
    std::cout << "Client connected with fd: " << fd << std::endl;
}

// Destructor
Client::~Client()
{
    std::cout << "Client disconnected: " << _nickname << " (fd: " << _clientFd << ")" << std::endl;
    close(_clientFd);
}

// ========== GETTERS ==========

int Client::getClientFd() const
{
    return _clientFd;
}

std::string Client::getNickname() const
{
    return _nickname;
}

bool Client::getIsAuthenticated() const
{
    return _isAuthenticated;
}

bool Client::getHasPassGiven() const
{
    return _hasPassGiven;
}

bool Client::getHasNickGiven() const
{
    return _hasNickGiven;
}

std::string Client::getInputBuffer()
{
    return _inputBuffer;
}

// ========== SETTERS ==========

void Client::setNickname(const std::string nickname)
{
    _nickname = nickname;
}

void Client::setUsername(const std::string username)
{
    _username = username;
}

void Client::setHostname(const std::string hostname)
{
    _hostname = hostname;
}

void Client::setIsAuthenticated(bool isAuthenticated)
{
    _isAuthenticated = isAuthenticated;
}

void Client::setHasPassGiven(bool hasPassGiven)
{
    _hasPassGiven = hasPassGiven;	std::vector<std::string> _channels;  // Lista de canales en los que está
}

void Client::setHasNickGiven(bool hasNickGiven)
{
    _hasNickGiven = hasNickGiven;
}

// ========== BUFFER ==========

void Client::setInputBuffer(const std::string inputBuffer)
{
    _inputBuffer = inputBuffer;
}

void Client::clearInputBuffer()
{
    _inputBuffer.clear();
}