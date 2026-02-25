/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parsing_Utils.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 14:13:14 by volmer            #+#    #+#             */
/*   Updated: 2026/02/17 15:25:10 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Client.hpp"
#include <algorithm>

/* Returns true if the buffer contains at least one complete command (\n). */
bool Client::hasAllCommand()
{
    return _inputBuffer.find("\n") != std::string::npos;
}

/* Appends raw recv() data to the buffer; data may arrive fragmented. */
void Client::addToBuffer(const std::string input)
{
    _inputBuffer += input;
}

/*
 * Extracts the first complete line from the buffer (up to \n, excluding it).
 * Strips a trailing \r if present. Removes the extracted line from the buffer.
 * Returns an empty string if no complete line is available.
 */
std::string Client::extractCommand()
{
    size_t pos = _inputBuffer.find("\n");
    if (pos == std::string::npos)
        return "";

    std::string command = _inputBuffer.substr(0, pos);

    if (!command.empty() && command[command.length() - 1] == '\r')
        command.erase(command.length() - 1);

    _inputBuffer.erase(0, pos + 1);
    return command;
}

/*
 * Extracts the first whitespace-delimited token from the buffer.
 * Removes the token and its delimiter. Returns the full buffer if no
 * delimiter is found.
 */
std::string Client::extractToken()
{
    size_t spacePos = _inputBuffer.find(' ');
    size_t crlfPos = _inputBuffer.find("\r\n");
    size_t pos = std::min(spacePos, crlfPos);

    if (pos == std::string::npos)
    {
        std::string token = _inputBuffer;
        _inputBuffer.clear();
        return token;
    }

    std::string token = _inputBuffer.substr(0, pos);
    _inputBuffer.erase(0, pos + 1);
    return token;
}
