/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:48:06 by sergio            #+#    #+#             */
/*   Updated: 2026/02/05 13:02:36 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"

Server::Server(int port, std::string &password)
    : _port(port), _password(password) {}

void Server::run() 
{
  std::cout << GREEN << "Starting IRC server on port " << _port << RESET
            << std::endl;
}