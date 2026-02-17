/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parsing_Utils.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 14:13:14 by volmer            #+#    #+#             */
/*   Updated: 2026/02/17 15:25:10 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"


/*
* Verifica si el buffer del cliente contiene un comando completo.
* Un comando IRC está completo cuando termina con la secuencia \r\n (CRLF) o \n (LF).
* Retorna true si hay al menos un comando completo en el buffer, false en caso contrario.
* Esta función es fundamental para determinar si podemos procesar un comando o
* si debemos esperar más datos del cliente.
*/
bool Client::hasAllCommand()
{
    return _inputBuffer.find("\n") != std::string::npos;
}

/*
* Añade los datos recibidos al buffer de entrada del cliente.
* Los datos recibidos de recv() pueden llegar fragmentados, por lo que
* se van acumulando en el buffer hasta formar comandos completos.
* Esta función simplemente concatena el nuevo input al buffer existente.
* El buffer se procesará posteriormente cuando se detecte un comando completo (\r\n).
*/
void Client::addToBuffer(const std::string input)
{
    _inputBuffer += input;
}

/*
* Extrae y retorna el primer comando completo del buffer del cliente.
* Busca la primera aparición de \n (LF) que marca el final de un comando.
* Acepta tanto \r\n (CRLF) como \n (LF) para compatibilidad con diferentes clientes.
* Pasos:
* 1. Busca la posición de \n en el buffer
* 2. Si no encuentra \n, retorna string vacío (comando incompleto)
* 3. Si encuentra \n:
*    - Extrae el texto desde el inicio hasta \n (sin incluir \n ni \r si existe)
*    - Elimina del buffer el comando extraído + el terminador
*    - Retorna el comando extraído
* Esto permite procesar múltiples comandos que lleguen juntos en el buffer.
*/
std::string Client::extractCommand()
{
    size_t pos = _inputBuffer.find("\n");
    if (pos == std::string::npos)
    {
        return "";
    }
    
    // Extraer el comando (sin incluir \n)
    std::string command = _inputBuffer.substr(0, pos);
    
    // Quitar \r final si existe (para manejar \r\n)
    if (!command.empty() && command[command.length() - 1] == '\r')
    {
        command.erase(command.length() - 1);
    }
    
    // Eliminar del buffer el comando + \n
    _inputBuffer.erase(0, pos + 1);
    return command;
}


/*
* Extrae y retorna el primer token (palabra) del buffer del cliente.
* Un token es una palabra delimitada por espacios o \r\n.
* Pasos:
* 1. Busca el primer espacio o \r\n en el buffer
* 2. Si no encuentra ninguno, retorna string vacío
* 3. Si encuentra delimitador:
*    - Extrae el texto desde el inicio hasta el delimitador
*    - Elimina del buffer el token extraído + el delimitador
*    - Retorna el token extraído
* Esta función se usa para parsear los parámetros individuales de un comando.
* Ejemplo: "NICK sergio" → extractToken() devuelve "NICK", luego "sergio"
*/
std::string Client::extractToken()
{
    size_t spacePos = _inputBuffer.find(' ');
    size_t crlfPos = _inputBuffer.find("\r\n");
    size_t pos = std::min(spacePos, crlfPos);
    
    if (pos == std::string::npos)
    {
        std::string token = _inputBuffer;
        _inputBuffer.clear();
        return token;
    }
    
    std::string token = _inputBuffer.substr(0, pos);
    _inputBuffer.erase(0, pos + 1);  // +1 para saltar el espacio
    return token;
}