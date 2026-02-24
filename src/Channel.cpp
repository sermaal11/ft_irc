/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/23 10:24:00 by sergio            #+#    #+#             */
/*   Updated: 2026/02/23 10:31:16 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Channel.hpp"
#include "../include/Client.hpp"

// Constructor: crea un canal con el nombre especificado
Channel::Channel(std::string name)
    : _name(name), _topic(""), _inviteOnly(false), _topicRestricted(false),
      _key(""), _userLimit(0) {
}

// Destructor
Channel::~Channel() {
}

// ========== GETTERS ==========

// Obtiene el nombre del canal
std::string Channel::getName() const { return _name; }

// Obtiene el topic del canal
std::string Channel::getTopic() const { return _topic; }

// Obtiene el número de miembros del canal
int Channel::getMemberCount() const { return _members.size(); }

// ========== SETTERS ==========

// Establece el topic del canal
void Channel::setTopic(const std::string topic) { _topic = topic; }

// ========== GESTIÓN DE MIEMBROS ==========

/*
 * Añade un cliente como miembro del canal.
 * Si el cliente ya es miembro, no se duplica (el mapa sobrescribe).
 */
void Channel::addMember(Client *client) {
  _members[client->getClientFd()] = client;
}

/*
 * Elimina un miembro del canal por su file descriptor.
 * También lo elimina de la lista de operadores si lo es.
 */
void Channel::removeMember(int fd) {
  _members.erase(fd);
  _operators.erase(fd);
}

/*
 * Verifica si un file descriptor corresponde a un miembro del canal.
 */
bool Channel::isMember(int fd) const {
  return _members.find(fd) != _members.end();
}

// ========== GESTIÓN DE OPERADORES ==========

/*
 * Añade un cliente como operador del canal.
 * El cliente debe ser también miembro del canal.
 */
void Channel::addOperator(Client *client) {
  _operators[client->getClientFd()] = client;
}

/*
 * Elimina un operador por su file descriptor.
 */
void Channel::removeOperator(int fd) { _operators.erase(fd); }

/*
 * Verifica si un file descriptor corresponde a un operador del canal.
 */
bool Channel::isOperator(int fd) const {
  return _operators.find(fd) != _operators.end();
}

// ========== MENSAJERÍA ==========

/*
 * Envía un mensaje a todos los miembros del canal.
 * excludeFd: file descriptor del cliente que NO debe recibir el mensaje
 *            (normalmente el emisor). Usar -1 para enviar a todos.
 */
void Channel::broadcastMessage(const std::string message, int excludeFd) {
  std::map<int, Client *>::iterator it;
  for (it = _members.begin(); it != _members.end(); ++it) {
    if (it->first != excludeFd) {
      ::send(it->first, message.c_str(), message.length(), 0);
    }
  }
}

// ========== UTILIDADES ==========

/*
 * Genera la lista de nicknames del canal para RPL_NAMREPLY (353).
 * Los operadores se prefijan con '@'.
 * Formato: "@operador1 miembro2 miembro3"
 */
std::string Channel::getMemberList() const {
  std::string list;
  std::map<int, Client *>::const_iterator it;
  for (it = _members.begin(); it != _members.end(); ++it) {
    if (!list.empty())
      list += " ";
    if (isOperator(it->first))
      list += "@";
    list += it->second->getNickname();
  }
  return list;
}

// ========== MODOS DEL CANAL ==========

bool Channel::getInviteOnly() const { return _inviteOnly; }
void Channel::setInviteOnly(bool inviteOnly) { _inviteOnly = inviteOnly; }

bool Channel::getTopicRestricted() const { return _topicRestricted; }
void Channel::setTopicRestricted(bool topicRestricted) {
  _topicRestricted = topicRestricted;
}

std::string Channel::getKey() const { return _key; }
void Channel::setKey(const std::string key) { _key = key; }

int Channel::getUserLimit() const { return _userLimit; }
void Channel::setUserLimit(int limit) { _userLimit = limit; }

// ========== LISTA DE INVITADOS ==========

void Channel::addInvite(const std::string nickname) {
  // Evitar duplicados
  if (!isInvited(nickname))
    _inviteList.push_back(nickname);
}

bool Channel::isInvited(const std::string nickname) const {
  for (size_t i = 0; i < _inviteList.size(); i++) {
    if (_inviteList[i] == nickname)
      return true;
  }
  return false;
}

void Channel::removeInvite(const std::string nickname) {
  for (std::vector<std::string>::iterator it = _inviteList.begin();
       it != _inviteList.end(); ++it) {
    if (*it == nickname) {
      _inviteList.erase(it);
      return;
    }
  }
}

/*
 * Genera un string con los modos activos del canal.
 * Formato: "+itk clave" o "+tl 10" etc.
 */
std::string Channel::getModeString() const {
  std::string modes = "+";
  std::string params;

  if (_inviteOnly)
    modes += "i";
  if (_topicRestricted)
    modes += "t";
  if (!_key.empty()) {
    modes += "k";
    params += " " + _key;
  }
  if (_userLimit > 0) {
    modes += "l";
    std::ostringstream oss;
    oss << _userLimit;
    params += " " + oss.str();
  }

  if (modes == "+")
    return "+";
  return modes + params;
}
