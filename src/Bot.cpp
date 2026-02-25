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
 * ModBot — server-side invisible moderator bot.
 * Filters banned words in channel messages with a 3-strike system:
 *   Strike 1-2 : private warning to the user, message blocked.
 *   Strike 3   : KICK from the channel, strikes reset to 0.
 * Messages are fabricated with ":ModBot" prefix; the bot is not a real client.
 */

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

/* Converts a string to lowercase for case-insensitive comparisons. */
static std::string toLower(const std::string &str) {
  std::string result = str;
  for (size_t i = 0; i < result.length(); i++)
    result[i] = std::tolower(result[i]);
  return result;
}

/* Returns true if message contains any banned word (case-insensitive). */
bool Server::botCheckBadWords(const std::string &message) {
  std::string lower = toLower(message);
  for (size_t i = 0; i < badWordsCount; i++) {
    if (lower.find(badWordsArr[i]) != std::string::npos)
      return true;
  }
  return false;
}

/*
 * Processes a channel message through the mod bot.
 * Returns true if the message was blocked (bad word detected).
 * Returns false if the message is clean and should be delivered normally.
 */
bool Server::botProcessMessage(Client *client, Channel *channel,
                               const std::string &target,
                               const std::string &message) {
  if (!botCheckBadWords(message))
    return false;

  int fd = client->getClientFd();
  std::string nick = client->getNickname();

  _botWarnings[fd]++;
  int strikes = _botWarnings[fd];

  if (strikes < 3) {
    std::string warnMsg = ":ModBot PRIVMSG " + target + " :" +
                          "\x03" "04\xe2\x9a\xa0\xef\xb8\x8f  " + nick + ", watch your language! " +
                          "Warning " + (strikes == 1 ? "1" : "2") +
                          "/3\x03\r\n";
    ::send(fd, warnMsg.c_str(), warnMsg.length(), 0);
  } else {
    std::string kickNotice = ":ModBot PRIVMSG " + target + " :" +
                             "\x03" "04\xf0\x9f\x94\xa8 " + nick +
                             " has been kicked for inappropriate language.\x03\r\n";
    channel->broadcastMessage(kickNotice, -1);

    std::string kickMsg = ":ModBot KICK " + target + " " + nick +
                          " :Inappropriate language (3/3 warnings)\r\n";
    channel->broadcastMessage(kickMsg, -1);
    channel->removeMember(fd);

    _botWarnings[fd] = 0;

    if (channel->getMemberCount() == 0) {
      std::map<std::string, Channel *>::iterator it = _channels.find(target);
      if (it != _channels.end()) {
        delete it->second;
        _channels.erase(it);
      }
    }
  }
  return true;
}
