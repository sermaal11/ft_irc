/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 09:13:18 by sergio            #+#    #+#             */
/*   Updated: 2026/02/05 09:23:43 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream> //std::cerr
#include <string>   //std::stoi

#define RED "\033[1;31m"
#define RESET "\033[0m"

int main(int argc, char **argv) 
{
  if (argc != 3) 
  {
    std::cerr << RED << "Usage: ./ircserv <port> <password>" << RESET << "\n";
    return (1);
  }

  int port = std::stoi(argv[1]);
  if (port <= 0 || port > 65535) 
  {
    std::cerr << RED << "Invalid port number" << RESET << "\n";
    return (1);
  }

  std::string password = argv[2];
  if (password.empty()) 
  {
    std::cerr << RED << "Invalid password" << RESET << "\n";
    return (1);
  }

  return (0);
}