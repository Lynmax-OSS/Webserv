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

std::string getReasonPhrase(int status);
std::string getMimeType(const std::string &path);
bool        readFileToString(const std::string &path, std::string &out);
std::string buildResponse(int status, const std::map<std::string, std::string> &headers, const std::string &body, bool keepAlive);
std::string buildErrorResponse(int status, const ServerConfig *server, bool keepAlive);

#endif