/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 22:36:15 by volmer            #+#    #+#             */
/*   Updated: 2026/02/16 22:36:57 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "Utils.hpp"

class Client 
{
	private:
		int 		_port;
		std::string _password;
		int			_serverFd;
	public:
		Client(int port, std::string &password);
		void run();
};

#endif