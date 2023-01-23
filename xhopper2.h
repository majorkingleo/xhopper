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
#ifndef XHOPPER_H
#define XHOPPER_H

#define MODE_INIT            1

#define MODE_NONE            0
#define MODE_PURE_PROXY      ( MODE_INIT <<  1 )
#define MODE_CLIENT          ( MODE_INIT <<  2 )
#define MODE_SERVER          ( MODE_INIT <<  3 )
#define MODE_FILETRANSFER    ( MODE_INIT <<  4 )
#define MODE_REVERSECONTACT  ( MODE_INIT <<  5 )
#define MODE_COMPRESS        ( MODE_INIT <<  6 )
#define MODE_UNCOMPRESS      ( MODE_INIT <<  7 )
#define MODE_DEBUG           ( MODE_INIT <<  8 )
#define MODE_TIMEOUT         ( MODE_INIT <<  9 )
#define MODE_PROXY_TEL       ( MODE_INIT << 10 )
#define MODE_PROXY_FTP       ( MODE_INIT << 11 )

#define SERVER_BUF_LEN       100
#define BUFFER_LEN           51200
#define BUFFER_SIZE          BUFFER_LEN
#define BUFFER_READ_SIZE     (BUFFER_SIZE - 500)

#define RETURN_SUCCESS           0
#define RETURN_PARSE_ERROR       1
#define RETURN_BIND_ERROR        2
#define RETURN_CONNECT_ERROR     3
#define RETURN_CANNOT_OPEN_FILE  4
#define RETURN_CONTINUE          5
#define RETURN_CONT_DIRTY        6
#define RETURN_ERROR            -1
#define RETURN_GOT_REQUEST      -2

struct _SETUP
{
  int mode;
  int timeout;

  unsigned long stat_read_data;
  unsigned long stat_write_data;

  union {
    struct {
      char server_name[SERVER_BUF_LEN];
      int  server_port;
      int  listen_port;
	  int  wait_time;
    } proxy;

    struct {
      char server_name[SERVER_BUF_LEN];
      int  server_port;
      int  listen_port;
      int  continues;
    } proxy_tel;

    struct {
      char server_name[SERVER_BUF_LEN];
      int  server_port;
      int  listen_port;
    } proxy_ftp;

    struct {
      int listen_port;
    } copy_server;

    struct {
      char server_name[SERVER_BUF_LEN];
      int  server_port;
      char file_name[SERVER_BUF_LEN];
    } copy_client;

    struct {
      int listen_port;
      int control_port;
    } reverse_client;

    struct {
      char client_name[SERVER_BUF_LEN];
      int  client_port;
      char server_name[SERVER_BUF_LEN];
      int  server_port;
    } reverse_server;

  } u;

};

struct FdNode
{
  int s_read;
  int s_write;
  int s_current;
  char server_name[SERVER_BUF_LEN];
  int  server_port;
  int  bind_port;
  struct FdNode *partner;
  struct FdNode *parent;
  struct List* send_queue_reader;
  struct List* send_queue_writer;
  struct Node* root_reader;
  struct Node* root_writer;
};


extern struct _SETUP SETUP;

char *get_ad( void );

#ifdef HPUX
#  define NEED_SETENV
#endif

#ifdef SUNOS
#  define NEED_SETENV
#endif

#ifdef SINIX
#  define NEED_SETENV
#endif

#ifdef NEED_SETENV
int setenv(const char *name, const char *value, int overwrite);
#endif

#endif
