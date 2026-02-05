/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 13:00:00 by sergio            #+#    #+#             */
/*   Updated: 2026/02/05 14:57:05 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

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

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define RESET "\033[0m"

#endif
