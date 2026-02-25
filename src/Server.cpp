/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:48:06 by sergio            #+#    #+#             */
/*   Updated: 2026/02/25 11:22:08 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Channel.hpp"
#include "../include/Utils.hpp"

/*
 * Constructor.
 * Inicializa el servidor con el puerto y la contraseña proporcionados.
 */
Server::Server(int port, std::string &password)
    : _port(port), _password(password), _serverName("ircserv"), _serverFd(-1) {}
Server::~Server() {
  // Liberar todos los clientes
  std::map<int, Client *>::iterator cit;
  for (cit = _clients.begin(); cit != _clients.end(); ++cit) {
    ::close(cit->first);
    delete cit->second;
  }
  _clients.clear();

  // Liberar todos los canales
  std::map<std::string, Channel *>::iterator chit;
  for (chit = _channels.begin(); chit != _channels.end(); ++chit) {
    delete chit->second;
  }
  _channels.clear();

  // Cerrar el socket del servidor
  if (_serverFd >= 0)
    ::close(_serverFd);
}

// Mensaje de bienvenido cuando un usario se conecta al server
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

/*
 * Verifica si el cliente ha completado el registro.
 * Un cliente está registrado cuando:
 * 1. Ha enviado la contraseña correcta (isAuthenticated = true)
 * 2. Ha establecido un nickname (hasNickGiven = true)
 * 3. Ha enviado el comando USER (hasPassGiven = true
 *
 * Si el cliente acaba de completar el registro, se le envía el mensaje de
 * bienvenida.
 */
void Server::checkClientRegister(Client *client) {
  if (client->getIsAuthenticated() && client->getHasNickGiven() &&
      client->getHasUserGiven() && !client->getIsRegistered()) {
    client->setIsRegistered(true);
    sendWelcomeMessage(client);
  }
}

/*
 * Elimina un cliente del servidor y libera sus recursos.
 * Esta función se llama cuando un cliente se desconecta (por cierre de
 * conexión o error detectado por recv()). Proceso de limpieza:
 * 1. Busca el cliente en el mapa _clients usando su file descriptor
 *    - Si lo encuentra, libera la memoria del objeto Client (delete)
 *    - Elimina la entrada del mapa _clients
 * 2. Busca el file descriptor en el vector _pollFds
 *    - Recorre el vector hasta encontrar el fd correspondiente
 *    - Elimina la entrada del vector para que poll() no lo monitorice más
 *    - break: sale del bucle una vez encontrado (cada fd aparece una sola vez)
 * 3. Cierra el socket del cliente con close()
 * 4. Muestra mensaje de desconexión con el fd del cliente
 * Es fundamental realizar toda esta limpieza para evitar memory leaks y
 * que poll() no intente monitorizar sockets cerrados.
 */
void Server::removeClient(int fd, const std::string &reason) {
  // buscar cliente en el mapa
  std::map<int, Client *>::iterator it = _clients.find(fd);
  if (it != _clients.end()) {
    // Eliminar al cliente de todos los canales antes de borrarlo
    removeClientFromChannels(it->second, reason);
    delete it->second;
    _clients.erase(it);
  }
  // Limpiar strikes del bot
  _botWarnings.erase(fd);
  // buscar cliente en el vector de poll
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
 * Elimina un cliente de todos los canales en los que participa.
 * Se llama desde removeClient() cuando el cliente se desconecta.
 * Notifica a los demás miembros del canal de la desconexión.
 * Si el canal queda vacío después de eliminar al cliente, se destruye.
 */
void Server::removeClientFromChannels(Client *client, const std::string &reason) {
  int fd = client->getClientFd();
  std::string quitMsg =
      ":" + client->getNickname() + " QUIT :" + reason + "\r\n";

  std::map<std::string, Channel *>::iterator it = _channels.begin();
  while (it != _channels.end()) {
    if (it->second->isMember(fd)) {
      // Notificar a los demás miembros del canal
      it->second->broadcastMessage(quitMsg, fd);
      it->second->removeMember(fd);

      // Si el canal queda vacío, destruirlo
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
 * Maneja el comando QUIT.
 * Formato: QUIT [:mensaje]
 * Según RFC 2812:
 * 1. Envía ERROR al cliente antes de cerrar la conexión
 * 2. Notifica a todos los canales y elimina al cliente de ellos
 * 3. Cierra la conexión TCP completamente (close + delete)
 */
void Server::handleQuit(Client *client, const std::string &quitMessage) {
  std::string reason = quitMessage.empty() ? "Client Quit" : quitMessage;

  // Enviar ERROR al cliente antes de cerrar (RFC 2812)
  std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
  std::string errorMsg =
      "ERROR :Closing link (" + nick + ") [Quit: " + reason + "]\r\n";
  ::send(client->getClientFd(), errorMsg.c_str(), errorMsg.length(), 0);

  // removeClient() se encarga de:
  // - removeClientFromChannels() con broadcast del QUIT a los demás
  // - delete Client*, close(fd), limpiar _pollFds
  removeClient(client->getClientFd(), reason);
}

/*
 * Maneja el comando JOIN.
 * Formato: JOIN #canal
 * Pasos:
 * 1. Extrae el nombre del canal del buffer del cliente
 * 2. Verifica que el nombre empiece con '#'
 * 3. Si el canal no existe, lo crea y hace al cliente operador
 * 4. Si ya existe, añade al cliente como miembro
 * 5. Envía mensajes IRC de confirmación:
 *    - JOIN a todos los miembros del canal
 *    - RPL_TOPIC (332) si hay un topic
 *    - RPL_NAMREPLY (353) con la lista de miembros
 *    - RPL_ENDOFNAMES (366)
 */
void Server::handleJoin(Client *client) {
  std::string channelName = client->extractToken();

  // Verificar que se proporcionó un nombre de canal
  if (channelName.empty()) {
    std::string err = ":" + _serverName + " 461 " + client->getNickname() +
                      " JOIN :Not enough parameters\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  // Verificar que el nombre del canal empiece con '#'
  if (channelName[0] != '#') {
    std::string err = ":" + _serverName + " 403 " + client->getNickname() + " " +
                      channelName + " :No such channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  // Verificar si el canal ya existe
  bool isNew = false;
  if (_channels.find(channelName) == _channels.end()) {
    // Crear el canal si no existe — asignar a temporal primero para que
    // si new lanza, el mapa no quede con un nullptr huérfano
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

  // Si el cliente ya es miembro, no hacer nada
  if (channel->isMember(client->getClientFd()))
    return;

  // === Verificar restricciones de modos ===

  // Modo +i: invite-only - verificar si está invitado
  if (!isNew && channel->getInviteOnly()) {
    if (!channel->isInvited(client->getNickname())) {
      std::string err = ":" + _serverName + " 473 " + client->getNickname() + " " +
                        channelName + " :Cannot join channel (+i)\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }
  }

  // Modo +k: clave del canal
  if (!isNew && !channel->getKey().empty()) {
    std::string key = client->extractToken();
    if (key != channel->getKey()) {
      std::string err = ":" + _serverName + " 475 " + client->getNickname() + " " +
                        channelName + " :Cannot join channel (+k)\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }
  }

  // Modo +l: límite de usuarios
  if (!isNew && channel->getUserLimit() > 0) {
    if (channel->getMemberCount() >= channel->getUserLimit()) {
      std::string err = ":" + _serverName + " 471 " + client->getNickname() + " " +
                        channelName + " :Cannot join channel (+l)\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }
  }

  // Añadir al cliente como miembro
  channel->addMember(client);

  // Eliminar de la lista de invitados si estaba
  channel->removeInvite(client->getNickname());

  // Si es un canal nuevo, el creador es operador
  if (isNew)
    channel->addOperator(client);

  // === Enviar mensajes IRC de confirmación ===

  // 1. Notificar a TODOS los miembros del canal (incluido el que se une)
  std::string joinMsg =
      ":" + client->getNickname() + " JOIN " + channelName + "\r\n";
  channel->broadcastMessage(joinMsg, -1);

  // 2. Enviar el topic (RPL_TOPIC 332) si existe
  if (!channel->getTopic().empty()) {
    std::string topicMsg = ":" + _serverName + " 332 " + client->getNickname() + " " +
                           channelName + " :" + channel->getTopic() + "\r\n";
    ::send(client->getClientFd(), topicMsg.c_str(), topicMsg.length(), 0);
  }

  // 3. Enviar lista de miembros (RPL_NAMREPLY 353)
  // El '=' indica un canal público
  std::string namesMsg = ":" + _serverName + " 353 " + client->getNickname() + " = " +
                         channelName + " :" + channel->getMemberList() + "\r\n";
  ::send(client->getClientFd(), namesMsg.c_str(), namesMsg.length(), 0);

  // 4. Fin de la lista de nombres (RPL_ENDOFNAMES 366)
  std::string endMsg = ":" + _serverName + " 366 " + client->getNickname() + " " +
                       channelName + " :End of /NAMES list\r\n";
  ::send(client->getClientFd(), endMsg.c_str(), endMsg.length(), 0);
}

/*
 * Maneja el comando PRIVMSG.
 * Formato: PRIVMSG <target> :<mensaje>
 * <target> puede ser:
 *   - Un nombre de canal (#canal): el mensaje se reenvía a todos los miembros
 *   - Un nickname: el mensaje se envía directamente a ese usuario
 * Pasos:
 * 1. Parsea el target y el mensaje de la línea de parámetros
 * 2. Si target empieza con '#', busca el canal y hace broadcast
 * 3. Si no, busca al usuario por nickname y le envía el mensaje
 * 4. Envía errores IRC si el target no existe o faltan parámetros
 */
void Server::handlePrivmsg(Client *client, const std::string &params) {
  // Parsear: target <espacio> :mensaje
  if (params.empty()) {
    std::string err = ":" + _serverName + " 411 " + client->getNickname() +
                      " :No recipient given (PRIVMSG)\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  // Extraer target (primer token)
  size_t spacePos = params.find(' ');
  if (spacePos == std::string::npos) {
    std::string err =
        ":" + _serverName + " 412 " + client->getNickname() + " :No text to send\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  std::string target = params.substr(0, spacePos);
  std::string messageText = params.substr(spacePos + 1);

  // Si el mensaje empieza con ':', quitarlo (formato IRC)
  if (!messageText.empty() && messageText[0] == ':')
    messageText = messageText.substr(1);

  if (messageText.empty()) {
    std::string err =
        ":" + _serverName + " 412 " + client->getNickname() + " :No text to send\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  // Construir el mensaje IRC completo
  std::string fullMsg = ":" + client->getNickname() + " PRIVMSG " + target +
                        " :" + messageText + "\r\n";

  // === Mensaje a canal ===
  if (target[0] == '#') {
    std::map<std::string, Channel *>::iterator it = _channels.find(target);
    if (it == _channels.end()) {
      std::string err = ":" + _serverName + " 403 " + client->getNickname() + " " + target +
                        " :No such channel\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }
    Channel *channel = it->second;

    // Verificar que el cliente es miembro del canal
    if (!channel->isMember(client->getClientFd())) {
      std::string err = ":" + _serverName + " 404 " + client->getNickname() + " " + target +
                        " :Cannot send to channel\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }

    // === ModBot: filtrar palabras malsonantes ===
    if (botProcessMessage(client, channel, target, messageText))
      return; // Mensaje bloqueado por el bot

    // Enviar a todos los miembros del canal excepto al emisor
    channel->broadcastMessage(fullMsg, client->getClientFd());
  }
  // === Mensaje privado ===
  else {
    // Buscar al destinatario por nickname
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

    // Enviar el mensaje al destinatario
    ::send(targetClient->getClientFd(), fullMsg.c_str(), fullMsg.length(), 0);
  }
}

/*
 * Busca un cliente por nickname.
 * Recorre el mapa _clients y compara nicknames.
 * Retorna puntero al cliente si lo encuentra, NULL si no existe.
 */
Client *Server::findClientByNick(const std::string &nickname) {
  std::map<int, Client *>::iterator it;
  for (it = _clients.begin(); it != _clients.end(); ++it) {
    if (it->second->getNickname() == nickname)
      return it->second;
  }
  return NULL;
}

/*
 * Maneja el comando PART.
 * Formato: PART #canal [:mensaje]
 * El cliente sale del canal y se notifica a los demás miembros.
 */
void Server::handlePart(Client *client, const std::string &params) {
  if (params.empty()) {
    std::string err = ":" + _serverName + " 461 " + client->getNickname() +
                      " PART :Not enough parameters\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  // Extraer nombre del canal y mensaje opcional
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

  // Notificar a todos (incluido el que sale)
  std::string msg = ":" + client->getNickname() + " PART " + channelName;
  if (!partMsg.empty())
    msg += " :" + partMsg;
  msg += "\r\n";
  channel->broadcastMessage(msg, -1);

  channel->removeMember(client->getClientFd());

  // Si el canal queda vacío, destruirlo
  if (channel->getMemberCount() == 0) {
    delete channel;
    _channels.erase(it);
  }
}

/*
 * Maneja el comando KICK.
 * Formato: KICK #canal nickname [:razón]
 * Solo los operadores pueden expulsar usuarios.
 */
void Server::handleKick(Client *client, const std::string &params) {
  if (params.empty()) {
    std::string err = ":" + _serverName + " 461 " + client->getNickname() +
                      " KICK :Not enough parameters\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  // Parsear: #canal nickname [:razón]
  std::istringstream iss(params);
  std::string channelName, targetNick, reason;
  iss >> channelName >> targetNick;

  // Extraer razón si existe
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

  // Verificar que el que hace KICK es operador
  if (!channel->isOperator(client->getClientFd())) {
    std::string err = ":" + _serverName + " 482 " + client->getNickname() + " " +
                      channelName + " :You're not channel operator\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  // Buscar al usuario a expulsar
  Client *target = findClientByNick(targetNick);
  if (!target || !channel->isMember(target->getClientFd())) {
    std::string err = ":" + _serverName + " 441 " + client->getNickname() + " " +
                      targetNick + " " + channelName +
                      " :They aren't on that channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  // Notificar a todos los miembros del canal
  std::string kickMsg = ":" + client->getNickname() + " KICK " + channelName +
                        " " + targetNick + " :" + reason + "\r\n";
  channel->broadcastMessage(kickMsg, -1);

  // Eliminar al usuario del canal
  channel->removeMember(target->getClientFd());

  // Si el canal queda vacío, destruirlo
  if (channel->getMemberCount() == 0) {
    delete channel;
    _channels.erase(it);
  }
}

/*
 * Maneja el comando INVITE.
 * Formato: INVITE nickname #canal
 * Solo los operadores pueden invitar cuando el canal es +i.
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

  // El invitador debe ser miembro del canal
  if (!channel->isMember(client->getClientFd())) {
    std::string err = ":" + _serverName + " 442 " + client->getNickname() + " " +
                      channelName + " :You're not on that channel\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  // Si el canal es invite-only, solo operadores pueden invitar
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

  // Añadir a la lista de invitados y notificar
  channel->addInvite(targetNick);

  // Confirmar al invitador (RPL_INVITING 341)
  std::string confirmMsg = ":" + _serverName + " 341 " + client->getNickname() + " " +
                           targetNick + " " + channelName + "\r\n";
  ::send(client->getClientFd(), confirmMsg.c_str(), confirmMsg.length(), 0);

  // Notificar al invitado
  std::string inviteMsg = ":" + client->getNickname() + " INVITE " +
                          targetNick + " " + channelName + "\r\n";
  ::send(target->getClientFd(), inviteMsg.c_str(), inviteMsg.length(), 0);
}

/*
 * Maneja el comando TOPIC.
 * Formato: TOPIC #canal [:nuevo_topic]
 * Sin parámetro de topic: muestra el topic actual.
 * Con parámetro: cambia el topic (requiere operador si modo +t activo).
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

  // Si no hay parámetro de topic, mostrar el actual
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

  // Cambiar el topic
  // Si modo +t activo, solo operadores pueden cambiar
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

  // Notificar a todos los miembros
  std::string topicMsg = ":" + client->getNickname() + " TOPIC " + channelName +
                         " :" + newTopic + "\r\n";
  channel->broadcastMessage(topicMsg, -1);
}

/*
 * Maneja el comando MODE.
 * Formato: MODE #canal [+/-modos [parámetros]]
 * Modos soportados:
 *   i - invite-only
 *   t - topic restringido a operadores
 *   k - clave del canal (requiere parámetro)
 *   o - dar/quitar operador (requiere parámetro: nickname)
 *   l - límite de usuarios (requiere parámetro para +l)
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

  // Si el target no es un canal, ignorar (MODE para usuarios no implementado)
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

  // Sin modo: mostrar modos actuales (RPL_CHANNELMODEIS 324)
  if (modeStr.empty()) {
    std::string msg = ":" + _serverName + " 324 " + client->getNickname() + " " +
                      channelName + " " + channel->getModeString() + "\r\n";
    ::send(client->getClientFd(), msg.c_str(), msg.length(), 0);
    return;
  }

  // Verificar que es operador para cambiar modos
  if (!channel->isOperator(client->getClientFd())) {
    std::string err = ":" + _serverName + " 482 " + client->getNickname() + " " +
                      channelName + " :You're not channel operator\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  // Parsear los modos
  // Separamos modos añadidos y eliminados para construir el prefijo correctamente.
  // Ej: MODE #canal +ik-t genera "+ik-t clave", no "-ik" ni "+ikt"
  bool adding = true;
  std::string addedModes, removedModes, addedParams, removedParams;

  // Recoger todos los parámetros restantes
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

  // Notificar a todos los miembros del canal si se aplicaron modos
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
 * Inicia el servidor.
 * Crea un socket y lo configura para que escuche en el puerto especificado.
 */
int Server::createServerSocket() {
  /*
   * Crea un socket y lo configura para que escuche en el puerto especificado.
   * AF_INET: protocolo de red (IPv4). Indica uso de IPv4, suficiente y más
   * simple para este proyecto. SOCK_STREAM: tipo de socket (TCP). Indica
   * comunicación TCP orientada a conexión y fiable. 0: protocolo de transporte
   * (TCP). Elige automáticamente el protocolo adecuado. socket() devuelve un
   * file descriptor (int).
   */
  int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd < 0) {
    std::cerr << RED << "socket() failed: " << std::strerror(errno) << RESET
              << "\n";
    return (-1);
  }

  /*
   * Configura el socket para que pueda ser reutilizado.
   * SOL_SOCKET: nivel de socket. El nivel se obtiene de la librería
   * <sys/socket.h>. SO_REUSEADDR: opción de socket. Permite reutilizar la
   * dirección del socket. opt: valor de la opción. 1 = true, 0 = false. Esto
   * permite reutilizar el puerto aunque esté en TIME_WAIT.
   */
  int opt = 1;
  if (::setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::cerr << RED
              << "setsockopt(SO_REUSEADDR) failed: " << std::strerror(errno)
              << RESET << "\n";
    ::close(serverFd);
    return (-1);
  }

  /*
   * Configura el socket para que no bloquee.
   * fcntl: función para manipular descriptores de archivo. Devuelve -1 si
   * falla. F_SETFL: establece flags del descriptor de archivo. Las flags se
   * obtienen de la librería <fcntl.h>. O_NONBLOCK: flag para establecer el
   * socket en modo no bloqueante.
   */
  if (fcntl(serverFd, F_SETFL, O_NONBLOCK) < 0) {
    std::cerr << RED << "fcntl(O_NONBLOCK) failed: " << std::strerror(errno)
              << RESET << "\n";
    ::close(serverFd);
    return (-1);
  }

  return serverFd;
}

/*
 * Asocia el socket a una dirección y lo pone en modo escucha.
 * Retorna true si tiene éxito, false si falla.
 */
bool Server::bindAndListen() {
  /*
   * Asocia el socket a una dirección IP y puerto (bind).
   * sockaddr_in: estructura que contiene la información de la dirección.
   * sin_family: familia de direcciones (AF_INET para IPv4).
   * sin_addr.s_addr: dirección IP (INADDR_ANY acepta conexiones de cualquier
   * interfaz). sin_port: puerto en formato de red (htons convierte a
   * big-endian).
   */
  struct sockaddr_in address;
  std::memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(_port);

  if (::bind(_serverFd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    std::cerr << RED << "bind() failed: " << std::strerror(errno) << RESET
              << "\n";
    return false;
  }

  /*
   * Pone el socket en modo escucha (listen).
   * SOMAXCONN: número máximo de conexiones pendientes en la cola.
   */
  if (::listen(_serverFd, SOMAXCONN) < 0) {
    std::cerr << RED << "listen() failed: " << std::strerror(errno) << RESET
              << "\n";
    return false;
  }
  return true;
}

/*
 * Acepta una nueva conexion de cliente.
 * Añade el nuevo socker al vector de poll
 */

void Server::acceptNewClient() {
  /*
   * Acepta una nueva conexión de cliente.
   * Retorna nuevo fd para comunicar con el cliente.
   */
  int clientFd = ::accept(_serverFd, NULL, NULL);
  if (clientFd < 0) {
    std::cerr << RED << "accept() failed: " << std::strerror(errno) << RESET
              << "\n";
    return;
  }

  /*
   * Configurar el socker del cliente como no bloqueante
   */
  if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
    std::cerr << RED << "fcntl(O_NONBLOCK) on client failed" << RESET << "\n";
    ::close(clientFd);
    return;
  }
  /*
   * Crear nuevo objeto cliente y ponerlo en el map clave->valor
   */
  Client *newClient;
  try {
    newClient = new Client(clientFd);
  } catch (std::bad_alloc &) {
    std::cerr << RED << "Out of memory: cannot allocate new client" << RESET << "\n";
    ::close(clientFd);
    return;
  }
  _clients[clientFd] = newClient;

  /*
   * Añadir el cliente al vector de poll.
   */
  pollfd clientPollFd;
  clientPollFd.fd = clientFd;
  clientPollFd.events = POLLIN;
  clientPollFd.revents = 0;
  _pollFds.push_back(clientPollFd);

  std::cout << GREEN << "[+] Client connected (fd=" << clientFd << ")" << RESET << "\n";
}

/*
 * Inicia el servidor.
 * Crea un socket y lo configura para que escuche en el puerto especificado.
 */
void Server::run() {
  // Ignorar SIGPIPE: sin esto, un send() a un cliente desconectado
  // abruptamente mataría el proceso entero.
  signal(SIGPIPE, SIG_IGN);

  _serverFd = createServerSocket();
  if (_serverFd < 0)
    return;

  if (!bindAndListen()) {
    ::close(_serverFd);
    _serverFd = -1; // evitar double close en el destructor
    return;
  }

  std::cout << GREEN << "[SERVER] Listening on port " << _port << RESET << "\n";

  /*
   * Añadir el socket del servidor al vector de poll
   * POLLIN: monitorizar eventos de nuevas conexiones
   * revents: Eventos que ya ocurrieron
   * push_back: añadimos fs del servidor al vector primero
   */
  pollfd serverPollFd;
  serverPollFd.fd = _serverFd;
  serverPollFd.events = POLLIN;
  serverPollFd.revents = 0;
  _pollFds.push_back(serverPollFd);

  /*
   * LOOP PRINCIPAL DEL SERVER
   * poll() bloquea hasta ver actividad en algun fd
   * -1: timeout infinito
   */

  while (g_running) {
    // poll( 1. Puntero -> array de fds, 2. Tamañon del array, 3. Segundos: -1:
    // Tiempo de espera indefinido.)
    int pollCount = ::poll(&_pollFds[0], _pollFds.size(), -1);
    if (pollCount < 0) {
      // EINTR: poll() interrumpido por señal (ej: SIGINT) — salida limpia
      if (errno != EINTR)
        std::cerr << RED << "poll() failed: " << std::strerror(errno) << RESET
                  << "\n";
      break;
    }
    /*
     * Recorrer todos los fds para ver cual ha tenido actividad
     * Indice para no modificar el vector durante la iteracion
     */

    for (size_t i = 0; i < _pollFds.size(); i++) {
      // En Linux, POLLHUP/POLLERR pueden llegar sin POLLIN cuando un cliente
      // se desconecta abruptamente. Los tratamos igual que datos: handleClientData
      // llamará a recv() que devolverá 0 o -1 y disparará removeClient().
      if (_pollFds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
        // Si es el socket del server, es una nueva conexion
        if (_pollFds[i].fd == _serverFd) {
          acceptNewClient();
        }
        // Si no, es un cliente que envia datos.
        else {
          size_t sizeBefore = _pollFds.size();
          handleClientData(i);
          // Si el cliente fue eliminado, _pollFds se encogió:
          // el elemento en [i] es ahora el siguiente cliente.
          // Decrementamos i para que el for++ no lo salte.
          if (_pollFds.size() < sizeBefore)
            i--;
        }
      }
    }
  }
  // El destructor cierra _serverFd; no lo cerramos aquí para evitar double close
}