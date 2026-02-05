/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:48:06 by sergio            #+#    #+#             */
/*   Updated: 2026/02/05 16:55:53 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"

/*
* Constructor.
* Inicializa el servidor con el puerto y la contraseña proporcionados.
*/
Server::Server(int port, std::string &password)
    : _port(port), _password(password) {}

/*
* Inicia el servidor.
* Crea un socket y lo configura para que escuche en el puerto especificado.
*/
void Server::run() 
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
	std::cout << GREEN << "OK: server socket created for port " << _port << " (fd=" << serverFd << ")" << RESET << RED << " DELETE (DEBUG)" << RESET << "\n";

    ::close(serverFd);	// ! PROVISIONAL: solo valida socket()
}