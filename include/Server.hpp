/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sergio <sergio@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/05 12:43:31 by sergio            #+#    #+#             */
/*   Updated: 2026/02/05 13:01:46 by sergio           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

#include "../include/Utils.hpp"

class Server 
{
	private:
		int _port;
		std::string _password;
	  
	public:
    	Server(int port, std::string &password);
    	void run();
};

#endif