/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:48:06 by sergio            #+#    #+#             */
/*   Updated: 2026/02/18 15:38:55 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"


/*
* Constructor.
* Inicializa el servidor con el puerto y la contraseña proporcionados.
*/
Server::Server(int port, std::string &password)
    : _port(port), _password(password), _serverFd(-1) {}
Server::~Server() {}

/*
* Verifica si el cliente ha completado el registro.
* Un cliente está registrado cuando:
* 1. Ha enviado la contraseña correcta (isAuthenticated = true)
* 2. Ha establecido un nickname (hasNickGiven = true)
* 3. Ha enviado el comando USER (hasPassGiven = true
* 
* Si el cliente acaba de completar el registro, se le envía el mensaje de bienvenida.
*/
void Server::checkClientRegister(Client *client)
{
	if (client->getIsAuthenticated() && client->getHasNickGiven() && client->getHasUserGiven())
	{
		sendWelcomeMessage(client);
	}
}


/*
* Elimina un cliente del servidor y libera sus recursos.
* Esta función se llama cuando un cliente se desconecta (voluntariamente con QUIT
* o por error de conexión detectado por recv()).
* Proceso de limpieza:
* 1. Busca el cliente en el mapa _clients usando su file descriptor
*    - Si lo encuentra, libera la memoria del objeto Client (delete)
*    - Elimina la entrada del mapa _clients
* 2. Busca el file descriptor en el vector _pollFds
*    - Recorre el vector hasta encontrar el fd correspondiente
*    - Elimina la entrada del vector para que poll() no lo monitorice más
*    - break: sale del bucle una vez encontrado (cada fd aparece una sola vez)
* 3. Cierra el socket del cliente con close()
* 4. Muestra mensaje de desconexión con el fd del cliente
* Es fundamental realizar toda esta limpieza para evitar memory leaks y
* que poll() no intente monitorizar sockets cerrados.
*/
void	Server::removeClient(int fd)
{
	//buscar cliente en el mapa
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it != _clients.end())
	{
		delete it->second;
		_clients.erase(it);
	}
	//buscar cliente en el vector de poll
	for (size_t i = 0; i < _pollFds.size(); i++)
	{
		if (_pollFds[i].fd == fd)
		{
			_pollFds.erase(_pollFds.begin() + i);
			break;
		}
	}
	::close(fd);
	std::cout << YELLOW << "Client disconnected (fd=" << fd << ")" << RESET << "\n";
}

/*
* Inicia el servidor.
* Crea un socket y lo configura para que escuche en el puerto especificado.
*/
int	Server::createServerSocket() 
{
	/*
	* Crea un socket y lo configura para que escuche en el puerto especificado.
	* AF_INET: protocolo de red (IPv4). Indica uso de IPv4, suficiente y más simple para este proyecto.
	* SOCK_STREAM: tipo de socket (TCP). Indica comunicación TCP orientada a conexión y fiable.
	* 0: protocolo de transporte (TCP). Elige automáticamente el protocolo adecuado.
	* socket() devuelve un file descriptor (int).
	*/
	int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd < 0) 
	{
		std::cerr << RED << "socket() failed: " << std::strerror(errno)
		<< RESET << "\n";
		return (-1);
	}
	std::cout << GREEN << "OK: socket created (fd=" << serverFd << ")" << RESET << RED << " DELETE (DEBUG)" << RESET << "\n";
	
	/*
	* Configura el socket para que pueda ser reutilizado.
	* SOL_SOCKET: nivel de socket. El nivel se obtiene de la librería <sys/socket.h>.
	* SO_REUSEADDR: opción de socket. Permite reutilizar la dirección del socket.
	* opt: valor de la opción. 1 = true, 0 = false. Esto permite reutilizar el puerto aunque esté en TIME_WAIT.
	*/
	int opt = 1;
	if (::setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
	{
    	std::cerr << RED << "setsockopt(SO_REUSEADDR) failed: " << std::strerror(errno) << RESET << "\n";
    	::close(serverFd);
    	return (-1);
	}
	std::cout << GREEN << "OK: SO_REUSEADDR enabled" << RESET << RED << " DELETE (DEBUG)" << RESET << "\n";

	/*
	* Configura el socket para que no bloquee.
	* fcntl: función para manipular descriptores de archivo. Devuelve -1 si falla.
	* F_SETFL: establece flags del descriptor de archivo. Las flags se obtienen de la librería <fcntl.h>.
	* O_NONBLOCK: flag para establecer el socket en modo no bloqueante.
	*/
	if (fcntl(serverFd, F_SETFL, O_NONBLOCK) < 0) 
	{
    	std::cerr << RED << "fcntl(O_NONBLOCK) failed: " << std::strerror(errno) << RESET << "\n";
    	::close(serverFd);
    	return (-1);
	}

	std::cout << GREEN << "OK: socket set to non-blocking mode" << RESET << RED << " DELETE (DEBUG)" << RESET << "\n";
	return serverFd;
}

/*
* Asocia el socket a una dirección y lo pone en modo escucha.
* Retorna true si tiene éxito, false si falla.
*/
bool Server::bindAndListen() 
{
	/*
    * Asocia el socket a una dirección IP y puerto (bind).
    * sockaddr_in: estructura que contiene la información de la dirección.
    * sin_family: familia de direcciones (AF_INET para IPv4).
    * sin_addr.s_addr: dirección IP (INADDR_ANY acepta conexiones de cualquier interfaz).
    * sin_port: puerto en formato de red (htons convierte a big-endian).
    */
	struct sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(_port);

	if (::bind(_serverFd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		std::cerr << RED << "bind() failed: " << std::strerror(errno) << RESET << "\n";
		return false;
	}
	std::cout << GREEN << "OK: socket bound to port " << _port << RESET << RED << " DELETE (DEBUG)" << RESET << "\n";
	
	/*
    * Pone el socket en modo escucha (listen).
    * SOMAXCONN: número máximo de conexiones pendientes en la cola.
    */
	if (::listen(_serverFd, SOMAXCONN) < 0)
	{
		std::cerr << RED << "listen() failed: " << std::strerror(errno) << RESET << "\n";
		return false;
	}
	std::cout << GREEN << "OK: socket listening" << RESET << RED << " DELETE (DEBUG)" << RESET << "\n";
	return true;
}

/*
* Acepta una nueva conexion de cliente.
* Añade el nuevo socker al vector de poll
*/

void Server::acceptNewClient() 
{
	/*
	* Acepta una nueva conexión de cliente.
	* Retorna nuevo fd para comunicar con el cliente.
	*/
	int clientFd = ::accept(_serverFd, NULL, NULL);
	if (clientFd < 0) 
	{
		std::cerr << RED << "accept() failed: " << std::strerror(errno) << RESET << "\n";
		return;
	}

	/*
	* Configurar el socker del cliente como no bloqueante
	*/
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) 
	{
		std::cerr << RED << "fcntl(O_NONBLOCK) on client failed" << RESET << "\n";
		::close(clientFd);
		return;
	}
	/*
	* Crear nuevo objeto cliente y ponerlo en el map clave->valor
	*/
	Client* newClient = new Client(clientFd);
	_clients[clientFd]= newClient;
	
	/*
	* Añadir el cliente al vector de poll.
	*/
	pollfd clientPollFd;
	clientPollFd.fd = clientFd;
	clientPollFd.events = POLLIN;
	clientPollFd.revents = 0;
	_pollFds.push_back(clientPollFd);

	std::cout << GREEN << "OK: new client connected (fd=" << clientFd << ")" << RESET << RED << " DELETE (DEBUG)" << RESET << "\n";
}

/*
* Inicia el servidor.
* Crea un socket y lo configura para que escuche en el puerto especificado.
*/
void Server::run() 
{
    _serverFd = createServerSocket();
    if (_serverFd < 0)
        return;

	if (!bindAndListen())
	{
		::close(_serverFd);
		return;
	}
    
    std::cout << GREEN << "OK: server socket created for port " << _port << " (fd=" << _serverFd << ")" << RESET << RED << " DELETE (DEBUG)" << RESET << "\n";


	/*
	* Añadir el socket del servidor al vector de poll
	* POLLIN: monitorizar eventos de nuevas conexiones
	* revents: Eventos que ya ocurrieron
	* push_back: añadimos fs del servidor al vector primero
	*/
	pollfd	serverPollFd;
	serverPollFd.fd = _serverFd;
	serverPollFd.events = POLLIN;
	serverPollFd.revents = 0;
	_pollFds.push_back(serverPollFd);
	
	/*
	* LOOP PRINCIPAL DEL SERVER
	* poll() bloquea hasta ver actividad en algun fd
	* -1: timeout infinito
	*/

	// ! revisar
	while (true)
	{
		// poll( 1. Puntero -> array de fds, 2. Tamañon del array, 3. Segundos: -1: Tiempo de espera indefinido.)
		int pollCount = ::poll(&_pollFds[0], _pollFds.size(), -1);
		if (pollCount < 0)
		{
			std::cerr << RED << "poll() failed: " << std::strerror(errno) << RESET << "\n";
			break;
		}
		/*
		* Recorrer todos los fds para ver cual ha tenido actividad
		* Indice para no modificar el vector durante la iteracion 
		*/

		for (size_t i = 0; i < _pollFds.size(); i++)
		{
			// Verificamos si hay eventos en el fd del server
			if (_pollFds[i].revents & POLLIN)
			{
				// Si es el socket del server, es una nueva conexion
				if (_pollFds[i].fd == _serverFd)
				{
					acceptNewClient();
				}
				// Si no, es un cliente que envia datos.
				else
				{
					handleClientData(i);
				}
			}
		}
	}
	::close(_serverFd);
}