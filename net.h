/*
    xhopper - A simple portforwarding software
    Copyright (C) 2004 - 2005 by Martin Oberzalek kingleo@gmx.at

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef XHOPPER_NET_H
#define XHOPPER_NET_H

#ifdef AIX
#  define SIZE_T size_t
#elif LINUX
#  define SIZE_T socklen_t
#else
#  define SIZE_T int
#endif

int accept_socket( int sockfd );
int accept_socket2( int sockfd, char* ip_buffer );
int bind_socket( int port, int nonblocking );
void set_sock_nonblocking( int fd );
int connect_server( const char* host, int port );
struct List* what_todo_list_read( struct List *fd_s );
struct List* what_todo_list_write( struct List *fd_s );

#endif
