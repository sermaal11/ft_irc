/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 22:36:15 by volmer            #+#    #+#             */
/*   Updated: 2026/02/18 15:41:15 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "Utils.hpp"

/*
 * Clase Client.
 * Representa un cliente conectado al servidor IRC.
 * 
 * Miembros privados:
 * - _clientFd: file descriptor del socket del cliente.
 * - _nickname: apodo del cliente en el servidor.
 * - _username: nombre de usuario del cliente.
 * - _hostname: nombre del host del cliente.
 * - _isAuthenticated: indica si el cliente está autenticado.
 * - _hasPassGiven: indica si el cliente ha proporcionado la contraseña.
 * - _hasNickGiven: indica si el cliente ha proporcionado un nickname.
 * - _inputBuffer: buffer para almacenar datos recibidos del cliente.
 * 
 * Métodos públicos:
 * - Client(int fd): constructor.
 * - ~Client(): destructor.
 * 
 * Getters:
 * - getClientFd(): obtiene el file descriptor del cliente.
 * - getNickname(): obtiene el nickname del cliente.
 * - getIsAuthenticated(): verifica si el cliente está autenticado.
 * - getHasPassGiven(): verifica si se ha proporcionado la contraseña.
 * - getHasNickGiven(): verifica si se ha proporcionado el nickname.
 * 
 * Setters:
 * - setNickname(const std::string nickname): establece el nickname.
 * - setUsername(const std::string username): establece el username.
 * - setHostname(const std::string hostname): establece el hostname.
 * - setIsAuthenticated(bool isAuthenticated): establece el estado de autenticación.
 * - setHasPassGiven(bool hasPassGiven): establece si se ha dado la contraseña.
 * - setHasNickGiven(bool hasNickGiven): establece si se ha dado el nickname.
 * 
 * Métodos de buffer:
 * - setInputBuffer(const std::string inputBuffer): establece el buffer de entrada.
 * - getInputBuffer(): obtiene el contenido del buffer de entrada.
 * - clearInputBuffer(): limpia el buffer de entrada.
 * 
 * Métodos de parsing:
 * - addToBuffer(const std::string input): añade datos al buffer de entrada.
 * - hasAllCommand(): verifica si hay un comando completo (busca \r\n).
 * - extractCommand(): extrae el primer comando completo del buffer.
 */
class Client 
{
	private:
		int 		_clientFd;
		std::string	_nickname;
		std::string _username;
		std::string _hostname;
		bool		_isAuthenticated;
		bool		_hasPassGiven;
		bool		_hasNickGiven;
		std::string _inputBuffer;
	public:
		Client(int fd);
		~Client();

		//Getters
		int			getClientFd() const;
		std::string	getNickname() const;
		bool		getIsAuthenticated() const;
		bool		getHasPassGiven() const;
		bool		getHasNickGiven() const;
		bool		getHasUserGiven() const;
		
		//Setters
		void		setNickname(const std::string nickname);
		void		setUsername(const std::string username);
		void		setHostname(const std::string hostname);

		//Setters Autentificación
		void		setIsAuthenticated(bool isAuthenticated);
		void		setHasPassGiven(bool hasPassGiven);
		void		setHasNickGiven(bool hasNickGiven);
		
		//Buffer
		void		setInputBuffer(const std::string inputBuffer);
		std::string	getInputBuffer();
		void		clearInputBuffer();


		//Parsing
		void		addToBuffer(const std::string input);
		bool		hasAllCommand(); //busca \r\n
		std::string	extractCommand(); //extrae todo el mensaje a buffer
		std::string extractToken(); // extraer un token del mensaje
};

#endif