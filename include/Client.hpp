/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: volmer <volmer@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 22:36:15 by volmer            #+#    #+#             */
/*   Updated: 2026/02/16 22:46:40 by volmer           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "Utils.hpp"

class Client 
{
	private:
		int 		_clientFd;
		std::string	_nickname;
		std::string _username;
		std::string _hostname;
		bool		_isAuthenticated;
		bool		_hasPassGiven;
		bool		_hasNickGiven;
		std::string _inputBuffer;
	public:
		Client(int fd);
		~Client();

		//Getters
		int			getClientFd() const;
		std::string	getNickname() const;
		bool		getIsAuthenticated() const;
		bool		getHasPassGiven() const;
		bool		getHasNickGiven() const;
		
		//Setters
		void		setNickname(const std::string nickname);
		void		setUsername(const std::string username);
		void		setHostname(const std::string hostname);
		void		setIsAuthenticated(bool isAuthenticated);
		void		setHasPassGiven(bool hasPassGiven);
		void		setHasNickGiven(bool hasNickGiven);
		
		//Buffer
		void		setInputBuffer(const std::string inputBuffer);
		std::string	getInputBuffer();
		void		clearInputBuffer();
};

#endif