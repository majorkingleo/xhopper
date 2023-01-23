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
#include "copy.h"
#include "net.h"
#include "xhopper2.h"
#include "utils.h"
#include "compress.h"
#include "list.h"
#include "proxy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


static int get_control_connection( int s_control )
{
  int s_caccept = 0;

  if( ( s_caccept = accept_socket( s_control ) ) < 0 )
    {
      return -1;
    }

  debug( "conrol connection established\n" );

  return s_caccept;
}

static int request_channel( int s_caccept )
{
  char buffer[BUFFER_SIZE];

  memset( buffer, 0, HEADER_SIZE );
  
  SET_HEADER_FLAG( buffer, FLAG_REQUEST );
  SET_HEADER_LEN( buffer, 0 );

  debug( "requesting control connection\n" );
  
  if( write( s_caccept, buffer, HEADER_SIZE ) == HEADER_SIZE )
    return 1;
    
  printf( "couldn't request channel\n" );

  return -1;
}

struct CmpData
{
  int s_listen;
  int s_control;
  int s_caccept;
  struct List **fd_list;
};

static int rev_client_cmp_read( struct FdNode* node, void *data )
{
  int s_accept = 0;
  int s_tunnel = 0;
  int s_listen = ((struct CmpData*)(data))->s_listen;
  int s_control = ((struct CmpData*)(data))->s_control;
  int s_caccept = ((struct CmpData*)(data))->s_caccept;
  struct List **fd_list = ((struct CmpData*)(data))->fd_list;
 
  if( node->s_current == s_listen )
    {            
      if( ( ( s_accept = accept_socket( s_listen ) ) > 0 ) &&
	  ( request_channel( s_caccept ) ) &&
	  ( ( s_tunnel = get_control_connection( s_control ) ) > 0 ) )
	{
	  struct List *l = NULL;
	  
	  set_sock_nonblocking( s_accept );
	  set_sock_nonblocking( s_tunnel );
	  
	  l = list_add( fd_list );
	  l->data = node = calloc( 1, sizeof( struct FdNode ) );
	  
	  node->s_read = s_accept;
	  node->s_write = s_tunnel;		     
	  return 1;
	}  
      else 
	{
	  return RETURN_CONNECT_ERROR;
	}
    }

  return 0;
}

static int rev_client_read( struct FdNode* node, void *data )
{
  return reader_writer_read( node );
}

static int rev_client_write( struct FdNode* node, void *data )
{
  return reader_writer_write( node );
}

int create_reverse_client( void )
{
  int   s_listen = 0;
  int   s_control = 0;
  int   s_caccept = 0;
  int   ret;

  struct List*   fd_list = NULL;
  struct List*   l = NULL;
  struct FdNode* node = NULL;
  struct ProxyCallbacks cb;
  struct CmpData cd;

  memset( &cb, 0, sizeof( struct ProxyCallbacks ) );

  if( ( s_listen = bind_socket( SETUP.u.reverse_client.listen_port, 1 ) ) <= 0 )
    {
      return RETURN_BIND_ERROR;
    }
  
  if( ( s_control = bind_socket( SETUP.u.reverse_client.control_port, 0 ) ) <= 0 )
    {
      return RETURN_BIND_ERROR;
    }

  if( ( s_caccept = get_control_connection( s_control ) ) < 0 )
    return RETURN_BIND_ERROR;

  l = list_add( &fd_list );
  l->data = node = calloc( 1, sizeof( struct FdNode ) );
  node->s_read = s_listen;

  cd.s_listen = s_listen;
  cd.s_control = s_control;
  cd.s_caccept = s_caccept;
  cd.fd_list = &fd_list;

  cb.cmp_node_read = rev_client_cmp_read;
  cb.do_read = rev_client_read;
  cb.do_write = rev_client_write;

  cb.cmp_node_read_data = &cd;
  cb.do_write_data = NULL;
  cb.do_read_data = NULL;

  ret = proxy_func( cb, &fd_list );

  free_list( &fd_list );
  
  close( s_control );
  close( s_caccept );

  return ret;
}

static int rev_server_cmp_node_read( struct FdNode* node, void *data )
{
  return 0;
}

struct RevServerData
{
  int s_control;
  struct List **fd_list;
};

static int rev_server_do_read( struct FdNode* node, void *data )
{
  int rret = 0;
  int s_control = ((struct RevServerData*)data)->s_control;
  struct List **fd_list = ((struct RevServerData*)data)->fd_list;

  if( ( rret = reader_writer_read( node ) ) != RETURN_CONTINUE )
    {
      if( rret == RETURN_GOT_REQUEST && node->s_current == s_control )
	{
	  int s_tunnel = 0;
	  int s_server = 0;
	  struct List *l = NULL;
	  
	  printf( "connection request over control connection got\n" );
	  
	  if( ( s_tunnel = connect_server( SETUP.u.reverse_server.client_name, 
					   SETUP.u.reverse_server.client_port ) ) <= 0 ) {
	    return RETURN_BIND_ERROR;
	  }
	  
	  if( ( s_server = connect_server( SETUP.u.reverse_server.server_name, 
					   SETUP.u.reverse_server.server_port ) ) <= 0 ) {
	    return RETURN_BIND_ERROR;
	  }		
	  
	  set_sock_nonblocking( s_server );
	  set_sock_nonblocking( s_tunnel );
	  
	  l = list_add( fd_list );
	  l->data = node = calloc( 1, sizeof( struct FdNode ) );
	  
	  node->s_read = s_tunnel;
	  node->s_write = s_server;

	  return RETURN_CONTINUE;
	} 
      return rret;
    }
  return rret;
}

static int rev_server_do_write( struct FdNode* node, void *data )
{
  return reader_writer_write( node );
}

int create_reverse_server( void )
{
  int   s_control = 0;
  struct List*   fd_list = NULL;
  struct List*   l = NULL;
  struct FdNode* node = NULL;
  struct ProxyCallbacks cb;
  int             ret = 0;
  struct RevServerData sd;

  memset( &cb, 0, sizeof( cb ) );

  if( ( s_control = connect_server( SETUP.u.reverse_server.client_name, 
				    SETUP.u.reverse_server.client_port ) ) <= 0 )
    {
      return RETURN_CONNECT_ERROR;
    }

  l = list_add( &fd_list );
  l->data = node = calloc( 1, sizeof( struct FdNode ) );
  node->s_read = s_control;

  set_sock_nonblocking( s_control );

  sd.s_control = s_control;
  sd.fd_list = &fd_list;

  cb.cmp_node_read = rev_server_cmp_node_read;
  cb.do_write = rev_server_do_write;
  cb.do_read = rev_server_do_read;

  cb.cmp_node_read_data = NULL;
  cb.do_write_data = NULL;
  cb.do_read_data = &sd;

  ret = proxy_func( cb, &fd_list );

  free_list( &fd_list );
  
  close( s_control );

  return ret;
}
