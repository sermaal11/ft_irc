/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 09:13:18 by sergio            #+#    #+#             */
/*   Updated: 2026/02/25 11:19:05 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/Utils.hpp"

/*
* Manejo de señales.
* g_running es una variable global que se utiliza para indicar si el servidor
* debe seguir ejecutándose.
* sigintHandler es una función que se ejecuta cuando se recibe una señal SIGINT.
* SIGINT es una señal que se envía al proceso cuando se presiona Ctrl+C.
* signal() es una función que se utiliza para registrar una función de manejo de señales.
* volatile es un calificador que indica que la variable puede ser modificada por
* factores externos al programa.
* sig_atomic_t es un tipo de dato que indica que la variable es atómica.
*/
volatile sig_atomic_t g_running = 1;

static void sigintHandler(int) { g_running = 0; }

/*
 * Programa principal.
 * Inicia el servidor con el puerto y la contraseña proporcionados.
 */
int main(int argc, char **argv) 
{
	/*
	* Validación de los argumentos del programa.
	* El servidor debe iniciarse con exactamente dos argumentos:
	* un puerto de escucha y una contraseña de conexión.
	*/
	if (argc != 3) 
	{
		std::cerr << RED << "ERROR: Usage -> ./ft_irc <port> <password>" << RESET << "\n";
		return (1);
	}

	/*
	* Validación del puerto.
	* El puerto debe ser un número entero entre 0 y 65535.
	*/
	for (size_t i = 0; argv[1][i]; i++) 
	{
		if (!isdigit(argv[1][i])) 
		{
			std::cerr << RED << "<port> must contain only digits (0 to 65535)" << RESET << "\n";
			return (1);
		}
	}
	int port = std::atoi(argv[1]);
	if (port <= 0 || port > 65535)
	{
		std::cerr << RED << "Invalid port number" << RESET << "\n";
		return (1);
	}

	/*
	* Validación de la contraseña.
	* La contraseña debe ser una cadena de caracteres no vacía.
	*/
	std::string password = argv[2];
	if (password.empty()) 
	{
		std::cerr << RED << "Invalid password" << RESET << "\n";
		return (1);
	}

	/*
	* Inicialización del servidor.
	* Se crea un objeto Server con el puerto y la contraseña proporcionados.
	* Se inicia el servidor con el método run().
	*/
	signal(SIGINT, sigintHandler);

	Server server(port, password);
	server.run();

	return (0);
}