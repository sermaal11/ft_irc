/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 22:48:43 by volmer            #+#    #+#             */
/*   Updated: 2026/02/17 14:24:29 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Client.hpp"

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

// Destructor: cierra la conexión y libera recursos
Client::~Client()
{
    std::cout << "Client disconnected: " << _nickname << " (fd: " << _clientFd << ")" << std::endl;
    close(_clientFd);
}

// ========== GETTERS ==========

// Obtiene el file descriptor del cliente
int Client::getClientFd() const
{
    return _clientFd;
}

// Obtiene el nickname del cliente
std::string Client::getNickname() const
{
    return _nickname;
}

// Verifica si el cliente está autenticado
bool Client::getIsAuthenticated() const
{
    return _isAuthenticated;
}

// Verifica si el cliente ha proporcionado la contraseña
bool Client::getHasPassGiven() const
{
    return _hasPassGiven;
}

// Verifica si el cliente ha proporcionado un nickname
bool Client::getHasNickGiven() const
{
    return _hasNickGiven;
}

// Obtiene el contenido del buffer de entrada
std::string Client::getInputBuffer()
{
    return _inputBuffer;
}

// ========== SETTERS ==========

// Establece el nickname del cliente
void Client::setNickname(const std::string nickname)
{
    _nickname = nickname;
}

// Establece el username del cliente
void Client::setUsername(const std::string username)
{
    _username = username;
}

// Establece el hostname del cliente
void Client::setHostname(const std::string hostname)
{
    _hostname = hostname;
}

// Establece el estado de autenticación del cliente
void Client::setIsAuthenticated(bool isAuthenticated)
{
    _isAuthenticated = isAuthenticated;
}

// Marca si el cliente ha proporcionado la contraseña
void Client::setHasPassGiven(bool hasPassGiven)
{
    _hasPassGiven = hasPassGiven;
}

// Marca si el cliente ha proporcionado un nickname
void Client::setHasNickGiven(bool hasNickGiven)
{
    _hasNickGiven = hasNickGiven;
}

// ========== BUFFER ==========

// Establece el contenido del buffer de entrada
void Client::setInputBuffer(const std::string inputBuffer)
{
    _inputBuffer = inputBuffer;
}

// Limpia el buffer de entrada después de procesarlo
void Client::clearInputBuffer()
{
    _inputBuffer.clear();
}