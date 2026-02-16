/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:43:31 by sergio            #+#    #+#             */
/*   Updated: 2026/02/16 18:46:41 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

#include "../include/Utils.hpp"

/*
 * Clase Server.
 * Implementa un servidor IRC simple.
 * 
 * Miembros:
 * - _port: puerto en el que se ejecuta el servidor.
 * - _password: contraseña del servidor.
 * 
 * Métodos:
 * - Server(int port, std::string &password): constructor.
 * - run(): inicia el servidor.
 */
class Server 
{
	private:
		int _port;
		std::string _password;
		int	_serverFd;
		int	createServerSocket();
	  
	public:
    	Server(int port, std::string &password);
    	void run();
};

#endif