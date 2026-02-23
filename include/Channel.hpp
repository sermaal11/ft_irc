/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/23 10:24:00 by sergio            #+#    #+#             */
/*   Updated: 2026/02/23 10:30:51 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "Utils.hpp"

class Client;

/*
 * ============================================================================
 * Clase Channel
 * ============================================================================
 * Representa un canal IRC donde múltiples clientes pueden comunicarse.
 * Gestiona la lista de miembros, operadores y el topic del canal.
 *
 * ============================================================================
 * ATRIBUTOS PRIVADOS:
 * ============================================================================
 *
 * _name       : Nombre del canal (ej: "#general")
 * _topic      : Tema del canal (vacío por defecto)
 * _members    : Mapa fd -> Client* de todos los miembros del canal
 * _operators  : Mapa fd -> Client* de los operadores del canal
 *
 * ============================================================================
 */
class Channel {
private:
  // === INFORMACIÓN DEL CANAL ===
  std::string _name;
  std::string _topic;

  // === MODOS DEL CANAL ===
  bool _inviteOnly;      // Modo +i: solo invitados pueden unirse
  bool _topicRestricted; // Modo +t: solo operadores pueden cambiar topic
  std::string _key;      // Modo +k: contraseña del canal (vacío = sin clave)
  int _userLimit;        // Modo +l: límite de usuarios (0 = sin límite)

  // === MIEMBROS ===
  std::map<int, Client *> _members;     // fd -> Client*
  std::map<int, Client *> _operators;   // fd -> Client* (operadores)
  std::vector<std::string> _inviteList; // nicknames invitados (para modo +i)

public:
  // ========================================================================
  // CONSTRUCTOR Y DESTRUCTOR
  // ========================================================================
  Channel(std::string name);
  ~Channel();

  // ========================================================================
  // GETTERS
  // ========================================================================
  std::string getName() const;
  std::string getTopic() const;
  int getMemberCount() const;

  // ========================================================================
  // SETTERS
  // ========================================================================
  void setTopic(const std::string topic);

  // ========================================================================
  // GESTIÓN DE MIEMBROS
  // ========================================================================

  // Añade un miembro al canal
  void addMember(Client *client);

  // Elimina un miembro del canal por su fd
  void removeMember(int fd);

  // Verifica si un fd es miembro del canal
  bool isMember(int fd) const;

  // ========================================================================
  // GESTIÓN DE OPERADORES
  // ========================================================================

  // Añade un operador al canal
  void addOperator(Client *client);

  // Elimina un operador por su fd
  void removeOperator(int fd);

  // Verifica si un fd es operador del canal
  bool isOperator(int fd) const;

  // ========================================================================
  // MENSAJERÍA
  // ========================================================================

  // Envía un mensaje a todos los miembros del canal
  // excludeFd: fd del cliente que envía (no se le reenvía a él mismo), -1 para
  // enviar a todos
  void broadcastMessage(const std::string message, int excludeFd);

  // ========================================================================
  // UTILIDADES
  // ========================================================================

  // Genera la lista de nicknames del canal (para RPL_NAMREPLY 353)
  // Los operadores se prefijan con '@'
  std::string getMemberList() const;

  // ========================================================================
  // MODOS DEL CANAL
  // ========================================================================
  bool getInviteOnly() const;
  void setInviteOnly(bool inviteOnly);
  bool getTopicRestricted() const;
  void setTopicRestricted(bool topicRestricted);
  std::string getKey() const;
  void setKey(const std::string key);
  int getUserLimit() const;
  void setUserLimit(int limit);

  // ========================================================================
  // LISTA DE INVITADOS
  // ========================================================================
  void addInvite(const std::string nickname);
  bool isInvited(const std::string nickname) const;
  void removeInvite(const std::string nickname);

  // Genera string con los modos activos del canal (para respuestas MODE)
  std::string getModeString() const;
};

#endif
