/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:43:31 by sergio            #+#    #+#             */
/*   Updated: 2026/02/05 16:35:40 by sergio           ###   ########.fr       */
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
	  
	public:
    	Server(int port, std::string &password);
    	void run();
};

#endif