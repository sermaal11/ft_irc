/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parsing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 14:10:39 by volmer            #+#    #+#             */
/*   Updated: 2026/02/23 21:05:02 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"

/*
 * Procesa y ejecuta los comandos IRC recibidos del cliente.
 * Esta función actúa como un router de comandos, identificando el tipo de
 * comando y ejecutando la lógica correspondiente para cada uno. Los comandos
 * IRC siguen el formato: COMANDO parámetro1 parámetro2... Comandos
 * implementados:
 * - NICK: establece el nickname del cliente
 * - USER: establece el username del cliente
 * - PASS: autentica al cliente con la contraseña del servidor
 * - PING: responde con PONG para mantener la conexión viva
 * - PRIVMSG: envía mensajes privados (pendiente de implementación completa)
 */
void Server::proccesCommand(Client *client, std::string command) {
  // Comandos permitidos antes de completar el registro
  bool preAuthCmd = (command == "PASS" || command == "NICK" ||
                     command == "USER" || command == "PING" ||
                     command == "QUIT");
  if (!preAuthCmd && !client->getIsRegistered()) {
    std::string err = ":" + _serverName + " 451 * :You have not registered\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    return;
  }

  /*
   * Comando NICK: establece o cambia el nickname del cliente.
   * Formato: NICK <nickname>
   * Pasos:
   * 1. Extrae el siguiente parámetro del buffer (el nickname deseado)
   * 2. Asigna el nickname al cliente
   * 3. Marca que el cliente ha proporcionado un nickname
   * Este comando es necesario para la autenticación inicial del cliente en IRC.
   */
  if (command == "NICK") {
    std::string nickname = client->extractToken();
    if (nickname.empty()) {
      std::string err = ":" + _serverName + " 431 * :No nickname given\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    } else {
      // Validar formato del nickname según RFC 1459:
      // máx 9 chars, primer char letra, resto letras/dígitos/-[]\'`^{}
      bool valid = nickname.length() <= 9 && std::isalpha(nickname[0]);
      for (size_t i = 1; valid && i < nickname.length(); ++i) {
        char c = nickname[i];
        valid = std::isalnum(c) || c == '-' || c == '[' || c == ']' ||
                c == '\\' || c == '\'' || c == '`' || c == '^' ||
                c == '{' || c == '}';
      }
      if (!valid) {
        std::string err = ":" + _serverName + " 432 * " + nickname +
                          " :Erroneous nickname\r\n";
        ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      } else {
      // Verificar que el nickname no esté en uso por otro cliente
      Client *existing = findClientByNick(nickname);
      if (existing != NULL && existing->getClientFd() != client->getClientFd()) {
        std::string err = ":" + _serverName + " 433 * " + nickname +
                          " :Nickname is already in use\r\n";
        ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      } else {
        if (client->getIsRegistered()) {
          // Notificar el cambio de nick a todos los canales en los que está
          std::string oldNick = client->getNickname();
          std::string nickMsg = ":" + oldNick + " NICK " + nickname + "\r\n";
          bool notified = false;
          std::map<std::string, Channel *>::iterator chit;
          for (chit = _channels.begin(); chit != _channels.end(); ++chit) {
            if (chit->second->isMember(client->getClientFd())) {
              chit->second->broadcastMessage(nickMsg, -1);
              notified = true;
            }
          }
          // Si no estaba en ningún canal, notificarse a sí mismo igualmente
          if (!notified)
            ::send(client->getClientFd(), nickMsg.c_str(), nickMsg.length(), 0);
        }
        client->setNickname(nickname);
        client->setHasNickGiven(true);
        checkClientRegister(client);
      }
      }
    }
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
  else if (command == "USER") {
    if (client->getIsRegistered()) {
      std::string err = ":" + _serverName + " 462 " + client->getNickname() +
                        " :You may not reregister\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
      return;
    }
    std::string username = client->extractToken();
    client->setUsername(username);
    client->setHasUserGiven(true);
    checkClientRegister(client);
  }
  /*
   * Comando PASS: autentica al cliente con la contraseña del servidor.
   * Formato: PASS <password>
   * Pasos:
   * 1. Extrae la contraseña proporcionada por el cliente del buffer
   * 2. Compara la contraseña recibida con la contraseña del servidor
   * (_password)
   * 3. Si coinciden, marca al cliente como autenticado
   * Este comando debe enviarse ANTES de NICK y USER según el protocolo IRC.
   * La autenticación es necesaria para acceder a servidores protegidos con
   * contraseña. Si la contraseña es incorrecta, el cliente no quedará
   * autenticado y puede ser rechazado.
   */
  else if (command == "PASS") {
    std::string password = client->extractToken();
    if (password == _password) {
      client->setIsAuthenticated(true);
      checkClientRegister(client);
    } else {
      std::string err = ":" + _serverName + " 464 * :Password incorrect\r\n";
      ::send(client->getClientFd(), err.c_str(), err.length(), 0);
    }
  }
  /*
   * Comando PING: verifica que la conexión sigue activa (keep-alive).
   * Formato: PING <token> o PING :<token>
   * El servidor o cliente envía PING periódicamente para verificar
   * conectividad.
   * Respuesta correcta según RFC: PONG :<token> o solo PONG si no hay token
   */
  else if (command == "PING") {
    std::string token = client->getInputBuffer();
    
    // Limpiar espacios inicales
    size_t start = token.find_first_not_of(" \t\r\n");
    
    if (start == std::string::npos) {
      // Si no hay parámetro, repondemos PONG
      std::string pongReply = "PONG\r\n";
      ::send(client->getClientFd(), pongReply.c_str(), pongReply.length(), 0);
    } else {
      // Extraer token y limpiar espacios
      size_t end = token.find_first_of("\r\n", start);
      if (end != std::string::npos)
        token = token.substr(start, end - start);
      else
        token = token.substr(start);
      size_t lastNonSpace = token.find_last_not_of(" \t");
      if (lastNonSpace != std::string::npos)
        token = token.substr(0, lastNonSpace + 1);
      // Si el token empieza por : usarlo, sino, ponerlo
      std::string pongReply;
      if (!token.empty() && token[0] == ':')
        pongReply = "PONG " + token + "\r\n";
      else
        pongReply = "PONG :" + token + "\r\n";
      
      ::send(client->getClientFd(), pongReply.c_str(), pongReply.length(), 0);
    }
  }
  /*
   * Comando PRIVMSG: envía mensajes privados a un usuario o canal.
   * Formato: PRIVMSG <target> :<mensaje>
   * Donde <target> puede ser un nickname (mensaje privado) o #canal (mensaje a
   * canal). Pasos actuales:
   * 1. Extrae el destinatario (nickname o canal)
   * 2. Extrae el mensaje a enviar
   * ! TODO: implementar la lógica de envío:
   *   - Buscar el cliente/canal destinatario
   *   - Reenviar el mensaje al destinatario
   *   - Manejar errores si el destinatario no existe
   */
  else if (command == "PRIVMSG") {
    std::string params = client->getInputBuffer();
    handlePrivmsg(client, params);
  }
  /*
   * Comando JOIN: une al cliente a un canal.
   * Formato: JOIN #canal
   * Si el canal no existe, se crea y el cliente se convierte en operador.
   * Si ya existe, el cliente se añade como miembro.
   * Se notifica a todos los miembros del canal de la nueva incorporación.
   */
  else if (command == "JOIN") {
    handleJoin(client);
  } else if (command == "PART") {
    std::string params = client->getInputBuffer();
    handlePart(client, params);
  } else if (command == "KICK") {
    std::string params = client->getInputBuffer();
    handleKick(client, params);
  } else if (command == "INVITE") {
    std::string params = client->getInputBuffer();
    handleInvite(client, params);
  } else if (command == "TOPIC") {
    std::string params = client->getInputBuffer();
    handleTopic(client, params);
  } else if (command == "MODE") {
    std::string params = client->getInputBuffer();
    handleMode(client, params);
  } else if (command == "QUIT") {
    std::string quitMsg = client->getInputBuffer();
    // Limpiar el mensaje: quitar espacios iniciales y ':' inicial
    size_t start = quitMsg.find_first_not_of(" \t");
    if (start != std::string::npos) {
      quitMsg = quitMsg.substr(start);
      if (!quitMsg.empty() && quitMsg[0] == ':')
        quitMsg = quitMsg.substr(1);
    } else {
      quitMsg = "";
    }
    handleQuit(client, quitMsg);
    return; // El cliente fue eliminado, no continuar procesando
  } else {
    std::string err = ":" + _serverName + " 421 " + client->getNickname() +
                      " " + command + " :Unknown command\r\n";
    ::send(client->getClientFd(), err.c_str(), err.length(), 0);
  }
}

/*
 * Maneja los datos recibidos de un cliente conectado.
 * Esta función se llama cuando poll() detecta actividad (POLLIN) en un socket
 * de cliente. Proceso:
 * 1. Lee los datos del socket con recv() (hasta 512 bytes por el estándar IRC)
 * 2. Si bytesRead <= 0, el cliente se desconectó o hubo error -> removeClient()
 * 3. Si hay datos válidos:
 *    - Añade los datos al buffer del cliente (pueden llegar fragmentados)
 *    - Procesa todos los comandos completos que haya en el buffer
 *    - Un comando está completo cuando termina en \r\n (CRLF)
 *    - Llama a proccesCommand() para cada comando completo extraído
 * El uso de buffer permite manejar comandos que llegan en múltiples paquetes
 * TCP o múltiples comandos que llegan juntos en un solo paquete.
 */
void Server::handleClientData(int i) {
  char buffer[512];
  int clientFd = _pollFds[i].fd;
  int bytesRead = ::recv(_pollFds[i].fd, buffer, sizeof(buffer) - 1, 0);

  // Cliente desconectado o error
  if (bytesRead <= 0) {
    if (bytesRead != 0)
      std::cerr << RED << "recv() failed: " << std::strerror(errno) << RESET << "\n";
    removeClient(clientFd);
    return;
  }
  Client *client = _clients[clientFd];
  buffer[bytesRead] = '\0';
  client->addToBuffer(buffer);
  while (client->hasAllCommand()) {
    // Extrae LÍNEA COMPLETA (sin \r\n)
    std::string line =
        client->extractCommand(); // "USER sergio 0 * :Sergio Real Name"

    // Ignorar líneas vacías
    if (line.empty())
      continue;

    // Parsear la línea con istringstream
    std::istringstream iss(line);
    std::string command;
    iss >> command; // Extraer primer token "USER"

    // Ignorar si no hay comando
    if (command.empty())
      continue;

    // Extraer el resto de la línea (todos los parámetros)
    std::string restOfLine;
    if (iss.tellg() != -1) {
      size_t pos = static_cast<size_t>(iss.tellg());
      // Saltar espacios iniciales
      while (pos < line.length() && line[pos] == ' ')
        pos++;
      restOfLine = line.substr(pos);
    }

    // Poner los parámetros completos en el buffer temporal
    if (!restOfLine.empty()) {
      client->setInputBuffer(restOfLine);
    } else {
      client->clearInputBuffer();
    }

    proccesCommand(client, command);

    // Si el cliente fue eliminado durante el procesamiento, detener el loop
    // para evitar usar el puntero liberado (use-after-free)
    if (_clients.find(clientFd) == _clients.end())
      return;

    // Limpiar el buffer después de procesar
    client->clearInputBuffer();
  }
}