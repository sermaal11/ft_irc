/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parsing_Utils.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 14:13:14 by volmer            #+#    #+#             */
/*   Updated: 2026/02/17 14:14:44 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"


/*
* Verifica si el buffer del cliente contiene un comando completo.
* Un comando IRC está completo cuando termina con la secuencia \r\n (CRLF).
* Retorna true si hay al menos un comando completo en el buffer, false en caso contrario.
* Esta función es fundamental para determinar si podemos procesar un comando o
* si debemos esperar más datos del cliente.
*/
bool Client::hasAllCommand()
{
    return _inputBuffer.find("\r\n") != std::string::npos;
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
* Busca la primera aparición de \r\n (CRLF) que marca el final de un comando IRC.
* Pasos:
* 1. Busca la posición de \r\n en el buffer
* 2. Si no encuentra \r\n, retorna string vacío (comando incompleto)
* 3. Si encuentra \r\n:
*    - Extrae el texto desde el inicio hasta \r\n (sin incluir \r\n)
*    - Elimina del buffer el comando extraído + los 2 caracteres \r\n
*    - Retorna el comando extraído
* Esto permite procesar múltiples comandos que lleguen juntos en el buffer.
*/
std::string Client::extractCommand()
{
    size_t pos = _inputBuffer.find("\r\n");
    if (pos == std::string::npos)
    {
        return "";
    }
    
    std::string command = _inputBuffer.substr(0, pos);
    _inputBuffer.erase(0, pos + 2);
    return command;
}