/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yosherau <yosherau@student.42kl.edu.my>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 02:52:08 by yosherau          #+#    #+#             */
/*   Updated: 2026/11/16 14:06:07 by yosherau         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
# define CGI_HPP

# include "../../include/ParserHeader/HttpRequest.hpp"
# include "../../include/ConfigHeader/ServerConfig.hpp"
# include "../../include/ConfigHeader/LocationConfig.hpp"
# include "../../include/ParserHeader/ClientConnection.hpp"
# include <sys/types.h>

class CGI
{
    private:
        pid_t               child_pid;
        int                 pipe_stdin;
        int                 pipe_stdout;
        std::string         output;
        std::string         body;
        int                 client_fd;
        bool                keep_alive;
        time_t              start;
        size_t              body_written;
        const ServerConfig* server;

    public:
        CGI();
        pid_t   get_child_pid(void) const;
        void    set_child_pid(pid_t new_pid);
        int     get_pipe_stdin(void) const;
        void    set_pipe_stdin(int pipe_stdin);
        int     get_pipe_stdout(void) const;
        void    set_pipe_stdout(int pipe_stdout);
        const std::string &get_output(void) const;
        void    set_output(std::string new_output);
        const std::string &get_body(void) const;
        void    append_output(const char *buffer, ssize_t length);
        void    set_body(std::string new_body);
        int     get_client_fd(void) const;
        void    set_client_fd(int new_fd);
        bool    get_keep_alive(void) const;
        void    set_keep_alive(bool new_keep_alive);
        time_t  get_start(void) const;
        void    set_start(time_t new_start);
        size_t  get_body_written(void) const;
        void    set_body_written(size_t written);
        const   ServerConfig *get_server(void) const;
        void    set_server(const ServerConfig *new_server);
};

void parseCgiOutput(const std::string &output, int &statusCode,
                           std::string &contentType, std::string &body);

#endif