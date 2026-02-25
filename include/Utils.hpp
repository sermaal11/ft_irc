/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 13:00:00 by sergio            #+#    #+#             */
/*   Updated: 2026/02/17 15:44:22 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/* Common includes, ANSI color macros, and the global shutdown flag. */
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
#include <sstream>
#include <csignal>			// signal, SIGPIPE

extern volatile sig_atomic_t g_running;

#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define PURPLE  "\033[1;35m"
#define RESET   "\033[0m"
#define CYAN    "\033[1;36m"

#endif
