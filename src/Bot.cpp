/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/24 21:21:00 by volmer            #+#    #+#             */
/*   Updated: 2026/02/25 12:29:28 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Channel.hpp"

/*
 * ============================================================================
 * ModBot — Bot moderador server-side (invisible)
 * ============================================================================
 * Filtra palabras malsonantes en mensajes de canales.
 * Sistema de 3 strikes:
 *   Strike 1-2: Warning al usuario, mensaje bloqueado
 *   Strike 3:   KICK automático del canal + reset de strikes
 *
 * El bot NO es un cliente real — el servidor fabrica mensajes con
 * el prefijo :ModBot para simular su presencia.
 * ============================================================================
 */

// Lista de palabras prohibidas (case-insensitive)
static const char *badWordsArr[] = {
    "mierda", "puta", "joder", "cabron", "gilipollas",
    "hijo de puta", "hostia", "culo", "polla", "capullo",
    "mamon", "zorra", "pendejo", "verga", "chingar",
    "pinche", "culero", "marica", "cojones", "carajo",
    "gonorrea", "malparido", "hijueputa", "idiota", "imbecil",
    "subnormal", "retrasado", "bollera", "soplagaitas",
    "fuck", "shit", "bitch", "asshole", "bastard", "dick", "cunt",
    "damn", "whore", "slut", "motherfucker", "bullshit", "dumbass",
    "jackass", "nigger", "nigga", "faggot", "retard", "moron",
    "wanker", "twat", "prick", "arsehole", "bellend", "tosser",
    "cocksucker", "dipshit", "shithead", "douchebag", "therian", "furry"
};
static const size_t badWordsCount = sizeof(badWordsArr) / sizeof(badWordsArr[0]);

/*
 * Convierte un string a minúsculas para comparación case-insensitive.
 * Compatible con C++98 (sin std::transform con ::tolower).
 */
static std::string toLower(const std::string &str) {
  std::string result = str;
  for (size_t i = 0; i < result.length(); i++)
    result[i] = std::tolower(result[i]);
  return result;
}

/*
 * Comprueba si un mensaje contiene alguna palabra prohibida.
 * Retorna true si se detecta lenguaje inapropiado.
 */
bool Server::botCheckBadWords(const std::string &message) {
  std::string lower = toLower(message);
  for (size_t i = 0; i < badWordsCount; i++) {
    if (lower.find(badWordsArr[i]) != std::string::npos)
      return true;
  }
  return false;
}

/*
 * Procesa un mensaje de canal a través del ModBot.
 * Retorna true si el mensaje fue bloqueado (contiene bad words).
 * Retorna false si el mensaje es limpio y puede enviarse normalmente.
 *
 * Sistema de strikes:
 * - 1er/2do strike: warning al usuario, mensaje bloqueado
 * - 3er strike: KICK del canal, strikes reseteados a 0
 */
bool Server::botProcessMessage(Client *client, Channel *channel,
                               const std::string &target,
                               const std::string &message) {
  if (!botCheckBadWords(message))
    return false;

  int fd = client->getClientFd();
  std::string nick = client->getNickname();

  // Incrementar strikes
  _botWarnings[fd]++;
  int strikes = _botWarnings[fd];

  if (strikes < 3) {
    // Warning al usuario (solo visible para él)
    std::string warnMsg = ":ModBot PRIVMSG " + target + " :" +
                          "\x03" "04⚠️  " + nick + ", watch your language! " +
                          "Warning " + (strikes == 1 ? "1" : "2") +
                          "/3\x03\r\n";
    ::send(fd, warnMsg.c_str(), warnMsg.length(), 0);
  } else {
    // 3er strike: notificar al canal y KICK
    std::string kickNotice = ":ModBot PRIVMSG " + target + " :" +
                             "\x03" "04🔨 " + nick +
                             " has been kicked for inappropriate language.\x03\r\n";
    channel->broadcastMessage(kickNotice, -1);

    // KICK del canal
    std::string kickMsg = ":ModBot KICK " + target + " " + nick +
                          " :Inappropriate language (3/3 warnings)\r\n";
    channel->broadcastMessage(kickMsg, -1);
    channel->removeMember(fd);

    // Reset de strikes
    _botWarnings[fd] = 0;

    // Si el canal queda vacío, destruirlo
    if (channel->getMemberCount() == 0) {
      std::map<std::string, Channel *>::iterator it = _channels.find(target);
      if (it != _channels.end()) {
        delete it->second;
        _channels.erase(it);
      }
    }
  }
  return true; // Mensaje bloqueado
}
