/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jdelorme <jdelorme@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 09:13:18 by sergio            #+#    #+#             */
/*   Updated: 2026/02/25 16:05:16 by jdelorme         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"
#include <csignal>
#include <cstring>

volatile sig_atomic_t g_running = 1;

static void sigintHandler(int) { 
	g_running = 0;
}

/*
 * Programa principal.
 * Inicia el servidor con el puerto y la contraseña proporcionados.
 */
int main(int argc, char **argv) 
{

	if (argc != 3) 
	{
		std::cerr << RED << "ERROR: Usage -> ./ft_irc <port> <password>" << RESET << "\n";
		return (1);
	}
	for (size_t i = 0; argv[1][i]; i++) 
	{
		if (!isdigit(argv[1][i])) 
		{
			std::cerr << RED << "<port> must contain only digits (0 to 65535)" << RESET << "\n";
			return (1);
		}
	}
	int port = std::atoi(argv[1]);
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
	struct sigaction sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigintHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	
	if (sigaction(SIGINT, &sa, NULL) == -1) 
	{
		std::cerr << RED << "Failed to set up signal handler" << RESET << "\n";
		return (1);
	}

	Server server(port, password);
	server.run();

	return (0);
}