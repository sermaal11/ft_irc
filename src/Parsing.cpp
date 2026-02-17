/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parsing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 14:10:39 by volmer            #+#    #+#             */
/*   Updated: 2026/02/17 14:16:20 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"

/*
* Procesa y ejecuta los comandos IRC recibidos del cliente.
* Esta función actúa como un router de comandos, identificando el tipo de comando
* y ejecutando la lógica correspondiente para cada uno.
* Los comandos IRC siguen el formato: COMANDO parámetro1 parámetro2...
* Comandos implementados:
* - NICK: establece el nickname del cliente
* - USER: establece el username del cliente
* - PASS: autentica al cliente con la contraseña del servidor
* - QUIT: desconecta al cliente del servidor
* - PING: responde con PONG para mantener la conexión viva
* - PRIVMSG: envía mensajes privados (pendiente de implementación completa)
*/
void	Server::proccesCommand(Client* client, std::string command)
{
	/*
	* Comando NICK: establece o cambia el nickname del cliente.
	* Formato: NICK <nickname>
	* Pasos:
	* 1. Extrae el siguiente parámetro del buffer (el nickname deseado)
	* 2. Asigna el nickname al cliente
	* 3. Marca que el cliente ha proporcionado un nickname
	* Este comando es necesario para la autenticación inicial del cliente en IRC.
	*/
	if (command == "NICK")
	{
		std::string nickname = client->extractCommand();
		client->setNickname(nickname);
		client->setHasNickGiven(true);
		std::cout << GREEN << "OK: NICK command found" << nickname << ")" << RESET << RED << " DELETE (DEBUG)" << RESET << "\n";
	}
	/*
	* Comando USER: establece la información del usuario.
	* Formato: USER <username> <hostname> <servername> <realname>
	* Pasos:
	* 1. Extrae el username del buffer
	* 2. Asigna el username al cliente
	* 3. Marca que el cliente ha proporcionado sus credenciales de usuario
	* Este comando se envía durante la fase de registro/conexión inicial.
	* Junto con NICK y PASS, completa el proceso de autenticación del cliente.
	*/
	else if (command == "USER")
	{
		std::string username = client->extractCommand();
		client->setUsername(username);
		client->setHasPassGiven(true);
		std::cout << GREEN << "OK: USER command found" << username << RESET << RED << " DELETE (DEBUG)" << RESET << "\n";
	}
	/*
	* Comando PASS: autentica al cliente con la contraseña del servidor.
	* Formato: PASS <password>
	* Pasos:
	* 1. Extrae la contraseña proporcionada por el cliente del buffer
	* 2. Compara la contraseña recibida con la contraseña del servidor (_password)
	* 3. Si coinciden, marca al cliente como autenticado
	* Este comando debe enviarse ANTES de NICK y USER según el protocolo IRC.
	* La autenticación es necesaria para acceder a servidores protegidos con contraseña.
	* Si la contraseña es incorrecta, el cliente no quedará autenticado y puede ser rechazado.
	*/
	else if (command == "PASS")
	{
		std::string password = client->extractCommand();
		std::cout << GREEN << "OK: Password command found" <<  password  << RESET << RED << " DELETE (DEBUG)" << RESET << "\n";
		if (password == _password)
		{
			client->setIsAuthenticated(true);
		}
	}
	/*
	* Comando QUIT: desconecta al cliente del servidor de forma limpia.
	* Formato: QUIT :<mensaje de despedida>
	* El cliente solicita cerrar la conexión voluntariamente.
	* Pasos:
	* 1. Obtiene el file descriptor del cliente
	* 2. Llama a removeClient() que:
	*    - Elimina al cliente del mapa _clients
	*    - Elimina el fd del vector _pollFds
	*    - Cierra el socket del cliente
	*    - Libera la memoria del objeto Client
	*/
	else if (command == "QUIT")
	{
		removeClient(client->getClientFd());
	}
	/*
	* Comando PING: verifica que la conexión sigue activa (keep-alive).
	* Formato: PING :<servidor>
	* El servidor o cliente envía PING periódicamente para verificar conectividad.
	* Pasos:
	* 1. Extrae el parámetro del comando PING
	* 2. Verifica que el comando sea efectivamente PING
	* 3. Responde con "PONG" para confirmar que la conexión está viva
	* Esto previene que la conexión se cierre por timeout de inactividad.
	* ! TODO: mejorar la lógica, el if interno parece redundante.
	*/
	else if (command == "PING")
	{
		std::string pong = client->extractCommand();
		if (pong == "PING")
		{
			std::string pong = "PONG";	
			::send(client->getClientFd(), pong.c_str(), pong.length(), 0);
		}
	}
	/*
	* Comando PRIVMSG: envía mensajes privados a un usuario o canal.
	* Formato: PRIVMSG <target> :<mensaje>
	* Donde <target> puede ser un nickname (mensaje privado) o #canal (mensaje a canal).
	* Pasos actuales:
	* 1. Extrae el destinatario (nickname o canal)
	* 2. Extrae el mensaje a enviar
	* ! TODO: implementar la lógica de envío:
	*   - Buscar el cliente/canal destinatario
	*   - Reenviar el mensaje al destinatario
	*   - Manejar errores si el destinatario no existe
	*/
	else if (command == "PRIVMSG")
	{
		std::string target = client->extractCommand();
		std::string message = client->extractCommand();
	}
}

/*
* Maneja los datos recibidos de un cliente conectado.
* Esta función se llama cuando poll() detecta actividad (POLLIN) en un socket de cliente.
* Proceso:
* 1. Lee los datos del socket con recv() (hasta 512 bytes por el estándar IRC)
* 2. Si bytesRead <= 0, el cliente se desconectó o hubo error -> removeClient()
* 3. Si hay datos válidos:
*    - Añade los datos al buffer del cliente (pueden llegar fragmentados)
*    - Procesa todos los comandos completos que haya en el buffer
*    - Un comando está completo cuando termina en \r\n (CRLF)
*    - Llama a proccesCommand() para cada comando completo extraído
* El uso de buffer permite manejar comandos que llegan en múltiples paquetes TCP
* o múltiples comandos que llegan juntos en un solo paquete.
*/
void Server::handleClientData(int i) 
{
    char 	buffer[512];
	int		clientFd = _pollFds[i].fd;
    int		bytesRead = ::recv(_pollFds[i].fd, buffer, sizeof(buffer) - 1, 0);

    // Cliente desconectado o error
    if (bytesRead <= 0) 
    {
        if (bytesRead == 0)
            std::cout << YELLOW << "Client disconnected (fd=" << _pollFds[i].fd << ")" << RESET << "\n";
        else
            std::cerr << RED << "recv() failed: " << std::strerror(errno) << RESET << "\n";

        removeClient(clientFd);
        return;
    }
	Client* client = _clients[clientFd];
    // Mostrar datos recibidos
    buffer[bytesRead] = '\0';
    std::cout << CYAN << "Received from fd " << _pollFds[i].fd << ": " << buffer << RESET;
	client->addToBuffer(buffer);
	while (client->hasAllCommand())
	{
		std::string command = client->extractCommand();
		std::cout << CYAN << "Command from fd=" << clientFd << ": " << command << RESET;
		proccesCommand(client, command);
	}
}