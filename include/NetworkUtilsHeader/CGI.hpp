/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yosherau <yosherau@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 02:52:08 by yosherau          #+#    #+#             */
/*   Updated: 2026/09/25 11:35:27 by yosherau         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
# define CGI_HPP

# include "../../include/ParserHeader/HttpRequest.hpp"
# include "../../include/ConfigHeader/ServerConfig.hpp"
# include "../../include/ConfigHeader/LocationConfig.hpp"

std::string handleCGI(const HttpRequest &req, const ServerConfig *server, const LocationConfig *location, const std::string &scriptPath);
bool	runCGI(std::string cgi_path, std::string script_path, const HttpRequest &req, std::string &output);
void parseCgiOutput(const std::string &output, int &statusCode,
                           std::string &contentType, std::string &body);

#endif