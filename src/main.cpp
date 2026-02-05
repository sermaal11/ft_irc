/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 09:13:18 by sergio            #+#    #+#             */
/*   Updated: 2026/02/05 13:00:01 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"

int main(int argc, char **argv) 
{
  if (argc != 3) 
  {
    std::cerr << RED << "Usage: ./ft_irc <port> <password>" << RESET << "\n";
    return (1);
  }

  for (int i = 0; argv[1][i]; i++) 
  {
    if (!isdigit(argv[1][i])) 
	{
      std::cerr << RED << "<port> must contain only digits" << RESET << "\n";
      return (1);
    }
  }

  int port = std::atoi(argv[1]);
  if (port < 0 || port > 65535) 
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

  Server server(port, password);
  server.run();

  return (0);
}