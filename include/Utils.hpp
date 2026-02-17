/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 13:00:00 by sergio            #+#    #+#             */
/*   Updated: 2026/02/17 14:22:24 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/*
* Incluye todas las librerías necesarias para el servidor.
* Incluye las definiciones de constantes y macros necesarias para el servidor.
*/
#ifndef UTILS_HPP
# define UTILS_HPP

#include <cctype>			// isdigit
#include <cstdlib>			// std::atoi
#include <iostream>			// std::cerr, std::cout, std::endl
#include <string>			// std::string
#include <sys/types.h>		// socket, bind, listen, accept, connect, send, recv, close
#include <sys/socket.h>		// socket, bind, listen, accept, connect, send, recv, close
#include <unistd.h>     	// close
#include <cerrno>			// errno
#include <cstring>      	// strerror
#include <fcntl.h>      	// fcntl, O_NONBLOCK
#include <poll.h>      		// poll
#include <netinet/in.h>		// sockaddr
#include <vector>			// std::vector
#include <map>				// STL map

#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define PURPLE  "\033[1;35m"
#define RESET   "\033[0m"
#define CYAN    "\033[1;36m"

#endif
