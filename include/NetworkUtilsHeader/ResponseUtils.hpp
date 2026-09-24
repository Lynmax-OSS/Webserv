/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseUtils.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yosherau <yosherau@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/11 18:58:27 by yosherau          #+#    #+#             */
/*   Updated: 2026/09/11 20:21:20 by yosherau         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_UTILS_HPP
# define RESPONSE_UTILS_HPP

# include <string>
# include <map>
# include "../ConfigHeader/ServerConfig.hpp"
# include "../../include/ParserHeader/HttpRequest.hpp"

std::string getReasonPhrase(int status);
std::string getMimeType(const std::string &path);
bool        readFileToString(const std::string &path, std::string &out);
std::string buildResponse(int status, const std::map<std::string, std::string> &headers, const std::string &body, bool keepAlive);
std::string buildErrorResponse(int status, const ServerConfig *server, bool keepAlive);
std::string buildAutoIndexPage(const std::string &urlPath, const std::string &dirPath, const std::vector<std::string> &names);
bool		hasParentTraversal(const std::string &path);
std::string	resolvePath(const ServerConfig *server, const LocationConfig *location, const HttpRequest &req);
std::string getExtension(const std::string &path);

#endif