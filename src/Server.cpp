/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:48:06 by sergio            #+#    #+#             */
/*   Updated: 2026/02/16 19:01:43 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"
#include <string>
#include <netinet/in.h>

/*
* Constructor.
* Inicializa el servidor con el puerto y la contraseña proporcionados.
*/
Server::Server(int port, std::string &password)
    : _port(port), _password(password), _serverFd(-1) {}

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
		return;
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
    	return;
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
    	return;
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

    ::close(_serverFd);	// ! PROVISIONAL: solo valida socket()
}