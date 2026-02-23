/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 22:36:15 by volmer            #+#    #+#             */
/*   Updated: 2026/02/23 09:37:56 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Utils.hpp"

/*
 * ============================================================================
 * Clase Client
 * ============================================================================
 * Representa un cliente conectado al servidor IRC.
 * Gestiona la información del cliente, su estado de autenticación/registro,
 * y el buffer de comandos recibidos.
 *
 * ============================================================================
 * ATRIBUTOS PRIVADOS:
 * ============================================================================
 *
 * === Conexión ===
 * _clientFd        : File descriptor del socket del cliente
 *
 * === Información del cliente ===
 * _nickname        : Apodo del cliente en el servidor (ej: "sergio")
 * _username        : Nombre de usuario del sistema (ej: "sergio")
 * _hostname        : Hostname del cliente
 *
 * === Estado de registro (para checkClientRegister) ===
 * _isAuthenticated : true si la contraseña (PASS) fue correcta
 * _hasPassGiven    : true si el cliente envió el comando PASS
 * _hasNickGiven    : true si el cliente envió el comando NICK
 * _hasUserGiven    : true si el cliente envió el comando USER
 *
 * === Buffer de entrada ===
 * _inputBuffer     : Almacena datos recibidos hasta formar comandos completos
 *                    Los comandos IRC terminan en \r\n (CRLF)
 *
 * ============================================================================
 * MÉTODOS PÚBLICOS:
 * ============================================================================
 */
class Client {
private:
  // === CONEXIÓN ===
  int _clientFd;

  // === INFORMACIÓN DEL CLIENTE ===
  std::string _nickname;
  std::string _username;
  std::string _hostname;

  // === ESTADO DE REGISTRO ===
  bool _isAuthenticated; // PASS enviado correctamente
  bool _hasPassGiven;    // PASS enviado
  bool _hasNickGiven;    // NICK enviado

  // === BUFFER DE ENTRADA ===
  std::string _inputBuffer;

  // === USER enviado ===
  bool _hasUserGiven; // USER enviado

public:
  // ========================================================================
  // CONSTRUCTOR Y DESTRUCTOR
  // ========================================================================
  Client(int fd);
  ~Client();

  // ========================================================================
  // GETTERS - Información del cliente
  // ========================================================================
  int getClientFd() const;         // Retorna el file descriptor
  std::string getNickname() const; // Retorna el nickname del cliente

  // ========================================================================
  // GETTERS - Estado de registro
  // ========================================================================
  bool getIsAuthenticated() const; // ¿Ha enviado PASS correctamente?
  bool getHasPassGiven() const;    // ¿Ha enviado PASS?
  bool getHasNickGiven() const;    // ¿Ha enviado NICK?
  bool getHasUserGiven() const;    // ¿Ha enviado USER?

  // ========================================================================
  // GETTERS - Buffer
  // ========================================================================
  std::string getInputBuffer(); // Retorna el contenido del buffer

  // ========================================================================
  // SETTERS - Información del cliente
  // ========================================================================
  void setNickname(
      const std::string nickname); // Establece el nickname (comando NICK)
  void setUsername(
      const std::string username); // Establece el username (comando USER)
  void setHostname(const std::string hostname); // Establece el hostname

  // ========================================================================
  // SETTERS - Estado de registro
  // ========================================================================
  void setIsAuthenticated(
      bool isAuthenticated); // Marca autenticación (PASS correcto)
  void setHasPassGiven(bool hasPassGiven);       // Marca que envió PASS
  void setHasNickGiven(bool hasNickGiven);       // Marca que envió NICK
  void setHasUserGiven(const bool hasUserGiven); // Marca que envió USER

  // ========================================================================
  // GESTIÓN DE BUFFER - Manipulación directa
  // ========================================================================
  void setInputBuffer(
      const std::string inputBuffer); // Establece contenido del buffer
  void clearInputBuffer();            // Limpia el buffer completamente

  // ========================================================================
  // PARSING - Procesamiento de comandos IRC
  // ========================================================================

  // Añade datos recibidos al buffer (pueden llegar fragmentados)
  void addToBuffer(const std::string input);

  // Verifica si hay al menos un comando completo en el buffer (busca \r\n)
  bool hasAllCommand();

  // Extrae una línea completa del buffer (hasta \r\n, sin incluirlo)
  std::string extractCommand();

  // Extrae un token (palabra) del buffer (delimitado por espacio o \r\n)
  std::string extractToken();
};

#endif