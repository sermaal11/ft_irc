/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:43:31 by sergio            #+#    #+#             */
/*   Updated: 2026/02/18 15:44:37 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# include "Utils.hpp"
# include "Client.hpp"

/*
 * Clase Server.
 * Implementa un servidor IRC simple.
 * 
 * Miembros privados:
 * - _port: puerto en el que se ejecuta el servidor.
 * - _password: contraseña del servidor.
 * - _serverFd: file descriptor del socket del servidor.
 * - _pollFds: vector de file descriptors para monitorizar con poll.
 * - _clients: mapa que asocia file descriptors con objetos Client.
 * 
 * Métodos privados:
 * - createServerSocket(): crea y configura el socket del servidor.
 * - bindAndListen(): vincula el socket a un puerto y escucha conexiones.
 * - acceptNewClient(): acepta una nueva conexión de cliente.
 * - handleClientData(int i): procesa los datos recibidos de un cliente.
 * - removeClient(int fd): elimina un cliente y cierra su conexión.
 * - proccesCommand(Client* client, std::string command): procesa comandos IRC del cliente.
 * 
 * Métodos públicos:
 * - Server(int port, std::string &password): constructor.
 * - run(): inicia el servidor y el bucle principal de eventos.
 * - ~Server(): destructor.
 */
class Server 
{
	private:
		int 		_port;
		std::string _password;
		int			_serverFd;
		std::vector<pollfd> _pollFds; //Vector de fd para monitorizar con poll
		std::map<int, Client*> _clients;
		
		int			createServerSocket();
		bool		bindAndListen();
		void		acceptNewClient();
		void		handleClientData(int i);
		void		removeClient(int fd);
		void		proccesCommand(Client* client, std::string command);
	  
		//autetificacion del usuario
		void	checkClientRegister(Client *client);
		void	sendWelcomeMessage(Client *client);

	public:
    	Server(int port, std::string &password);
    	void run();
		~Server();
};

#endif