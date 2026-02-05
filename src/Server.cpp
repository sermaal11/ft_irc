/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:48:06 by sergio            #+#    #+#             */
/*   Updated: 2026/02/05 14:58:25 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"

Server::Server(int port, std::string &password)
    : _port(port), _password(password) {}

void Server::run() 
{
	int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd < 0) 
	{
		std::cerr << RED << "socket() failed: " << std::strerror(errno)
		<< RESET << "\n";
		return;
	}
	
	int opt = 1;
	if (::setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
	{
    	std::cerr << RED << "setsockopt(SO_REUSEADDR) failed: " << std::strerror(errno) << RESET << "\n";
    	::close(serverFd);
    	return;
	}

	if (fcntl(serverFd, F_SETFL, O_NONBLOCK) < 0) 
	{
    	std::cerr << RED << "fcntl(O_NONBLOCK) failed: " << std::strerror(errno) << RESET << "\n";
    	::close(serverFd);
    	return;
	}	
	
	std::cout << GREEN << "OK: SO_REUSEADDR enabled" << RESET << "\n";
	std::cout << GREEN << "OK: server socket created for port " << _port << " (fd=" << serverFd << ")" << RESET << "\n";

    ::close(serverFd);	// ! PROVISIONAL: solo valida socket()
}