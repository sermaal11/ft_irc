/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:43:31 by sergio            #+#    #+#             */
/*   Updated: 2026/02/23 23:28:53 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "Channel.hpp"
#include "Client.hpp"
#include "Utils.hpp"

/*
 * ============================================================================
 * Clase Server
 * ============================================================================
 * Implementa un servidor IRC completo que gestiona múltiples clientes
 * conectados simultáneamente utilizando sockets TCP y multiplexación con
 * poll().
 *
 * Flujo de trabajo del servidor:
 * 1. Crear socket del servidor (socket, setsockopt, fcntl)
 * 2. Vincular a puerto y escuchar (bind, listen)
 * 3. Loop principal con poll() esperando eventos
 * 4. Aceptar nuevos clientes (accept)
 * 5. Procesar datos de clientes existentes (recv)
 * 6. Gestionar registro y autenticación IRC (PASS, NICK, USER)
 *
 * ============================================================================
 * ATRIBUTOS PRIVADOS:
 * ============================================================================
 *
 * === Configuración del servidor ===
 * _port            : Puerto en el que escucha el servidor (ej: 6667)
 * _password        : Contraseña requerida para conectarse (comando PASS)
 *
 * === Conexión y sockets ===
 * _serverFd        : File descriptor del socket principal del servidor
 * _pollFds         : Vector de estructuras pollfd para monitorizar eventos
 *                    con poll(). Incluye el fd del servidor + todos los
 * clientes
 *
 * === Gestión de clientes ===
 * _clients         : Mapa que relaciona file descriptors con objetos Client*
 *                    Permite acceso rápido a cualquier cliente por su fd
 *
 * ============================================================================
 * MÉTODOS PRIVADOS:
 * ============================================================================
 */
class Server {
private:
  // === CONFIGURACIÓN ===
  int _port;             // Puerto del servidor (6667 típicamente)
  std::string _password; // Contraseña para PASS
  std::string _serverName; // Nombre del servidor (ej: "ft_irc")

  // === CONEXIÓN ===
  int _serverFd;                // Socket principal del servidor
  std::vector<pollfd> _pollFds; // FDs monitorizados por poll()

  // === CLIENTES ===
  std::map<int, Client *> _clients; // fd -> Client*

  // === CANALES ===
  std::map<std::string, Channel *> _channels; // nombre -> Channel*

  // ========================================================================
  // INICIALIZACIÓN DEL SERVIDOR
  // ========================================================================

  // Crea el socket del servidor (socket, setsockopt, fcntl)
  // Configura: SO_REUSEADDR y O_NONBLOCK
  // Retorna: fd del socket o -1 si error
  int createServerSocket();

  // Vincula el socket a una dirección y lo pone en modo escucha
  // Usa: bind() con INADDR_ANY, listen() con SOMAXCONN
  // Retorna: true si éxito, false si error
  bool bindAndListen();

  // ========================================================================
  // GESTIÓN DE CONEXIONES
  // ========================================================================

  // Acepta una nueva conexión de cliente
  // Usa: accept(), fcntl(O_NONBLOCK)
  // Crea: nuevo objeto Client y lo añade a _clients y _pollFds
  void acceptNewClient();

  // Procesa datos recibidos de un cliente
  // Parámetro i: índice en _pollFds (NO el fd directamente)
  // Usa: recv() para leer datos, addToBuffer(), extractCommand()
  // Llama: proccesCommand() para cada comando completo
  void handleClientData(int i);

  // Elimina un cliente y cierra su conexión
  // Limpia: _clients (delete), _pollFds (erase), close(fd)
  void removeClient(int fd, const std::string &reason = "Client disconnected");

  // ========================================================================
  // PROCESAMIENTO DE COMANDOS IRC
  // ========================================================================

  // Router de comandos: identifica y ejecuta el comando recibido
  // Comandos implementados: PASS, NICK, USER, PING, PRIVMSG
  void proccesCommand(Client *client, std::string command);

  // Maneja el comando JOIN: une al cliente a un canal
  // Si el canal no existe, lo crea y hace al cliente operador
  void handleJoin(Client *client);

  // Maneja el comando PRIVMSG: envía mensajes a canales o usuarios
  // Recibe la línea completa de parámetros (target + mensaje)
  void handlePrivmsg(Client *client, const std::string &params);

  // Maneja el comando PART: sale de un canal
  void handlePart(Client *client, const std::string &params);

  // Maneja el comando KICK: expulsa a un usuario del canal (requiere operador)
  void handleKick(Client *client, const std::string &params);

  // Maneja el comando INVITE: invita a un usuario al canal (requiere operador)
  void handleInvite(Client *client, const std::string &params);

  // Maneja el comando TOPIC: muestra o cambia el tema del canal
  void handleTopic(Client *client, const std::string &params);

  // Maneja el comando MODE: cambia modos del canal (i, t, k, o, l)
  void handleMode(Client *client, const std::string &params);

  // Maneja el comando QUIT: cierra la conexión del cliente (RFC 2812)
  void handleQuit(Client *client, const std::string &quitMessage);

  // Busca un cliente por nickname, retorna NULL si no existe
  Client *findClientByNick(const std::string &nickname);

  // Elimina un cliente de todos los canales en los que está
  // Se llama desde removeClient() al desconectarse
  void removeClientFromChannels(Client *client, const std::string &reason = "Client disconnected");

  // ========================================================================
  // AUTENTICACIÓN Y REGISTRO
  // ========================================================================

  // Verifica si el cliente completó el registro (PASS + NICK + USER)
  // Si está completo, envía mensaje de bienvenida
  void checkClientRegister(Client *client);

  // Envía mensaje de bienvenida IRC (código 001)
  // Formato: ":server 001 nickname :Welcome..."
  void sendWelcomeMessage(Client *client);

public:
  // ========================================================================
  // CONSTRUCTOR Y DESTRUCTOR
  // ========================================================================
  Server(int port, std::string &password);
  ~Server();

  // ========================================================================
  // EJECUCIÓN DEL SERVIDOR
  // ========================================================================

  // Inicia el servidor y el bucle principal de eventos
  // Llama: createServerSocket(), bindAndListen(), poll() en loop
  void run();
};

#endif