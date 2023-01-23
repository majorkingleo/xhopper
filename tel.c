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
#include "tel.h"
#include "proxy.h"
#include "utils.h"
#include "net.h"
#include "list.h"
#include "xhopper2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>


struct cmp_data
{
  int s_listen;
  struct List **fd_list;
};

static void send_info( int s_server, struct FdNode* n1, int i )
{
  struct List* l = NULL;
  struct sockaddr_in addresse;
  int  addr_len = sizeof( addresse );
  struct SendNode *sn = 0;
  char buffer[SERVER_BUF_LEN];

  if( getsockname( s_server, (struct sockaddr*) &addresse, &addr_len ) == 0 )
    {
      l = list_append( &n1->send_queue_reader );
      l->data = sn = calloc( 1, sizeof( struct SendNode ) );
      sn->pcurrent = sn->buffer;
      strcpy( buffer, inet_ntoa(addresse.sin_addr) ); 
	  
      sprintf( sn->buffer, 
	       "#     #  #     #  #######  ######   ######   #######  ######\r\n"
	       " #   #   #     #  #     #  #     #  #     #  #        #     #\r\n"
	       "  # #    #     #  #     #  #     #  #     #  #        #     #\r\n"
	       "   #     #######  #     #  ######   ######   #####    ######\r\n"
	       "  # #    #     #  #     #  #        #        #        #   #\r\n"
	       " #   #   #     #  #     #  #        #        #        #    #\r\n"
	       "#     #  #     #  #######  #        #        #######  #     #\r\n"
	       "===============================================================\r\n"
	       "\tBack connection created\r\n"
	       "\tListening on %s on Port %d\r\n"
	       "\texport DISPLAY=%s:%d\r\n"
	       "\tsetenv DISPLAY %s:%d\r\n"
	       "===============================================================\r\n%s\r\n",
	       buffer, i,
	       buffer, i-6000,
	       buffer, i-6000,
	       get_ad() );
      
      sn->len = strlen( sn->buffer );
    }
}

static int proxy_tel_cmp_node_read( struct FdNode *node, void *data )
{
  int s_listen = ((struct cmp_data*)data)->s_listen;
  struct List **fd_list = ((struct cmp_data*)data)->fd_list;
  
  if( node->s_current == s_listen )
    {
      int s_server = 0;
      int s_accept = 0;
      char buffer[SERVER_BUF_LEN];
      
      if( ( ( s_accept = accept_socket2( s_listen, buffer ) ) > 0 ) &&		 
	  ( s_server = connect_server( SETUP.u.proxy_tel.server_name, SETUP.u.proxy_tel.server_port ) ) > 0 )
	{
	  struct List*   l = NULL;
	  struct FdNode *n1 = NULL;
	  int            i;
	  int            fd;

	  set_sock_nonblocking( s_accept );
	  set_sock_nonblocking( s_server );

	  l = list_add( fd_list );
	  l->data = n1 = calloc( 1, sizeof( struct FdNode ) );
	  
	  n1->s_read = s_accept;
	  n1->s_write = s_server;

	  for( l = *fd_list; l != NULL; l = l->next )
	    {
	      struct FdNode *n = (struct FdNode*) l->data;
	      
	      if( n == NULL || n->server_port == 0 )
		continue;

	      if( strcmp( n->server_name, buffer ) == 0 )
		{
		  printf( "backserver for ip: %s already installed\n", buffer );
		  n1->partner = n;
		  send_info( s_server, n1, n->bind_port );
		  return 1;
		}
	    }

	  for( i = 6000; i < 6050; i++ )
	    {
	      if( ( fd = bind_socket( i, 1 ) ) > 0 )
		break;
	    }

	  if( fd > 0 )
	    {
	      struct FdNode *n2 = NULL;

	      l = list_add( fd_list );
	      l->data = n2 = calloc( 1, sizeof( struct FdNode ) );

	      n2->s_read = fd;
	      strcpy( n2->server_name, buffer );
	      n2->server_port = 6000;
	      n2->bind_port = i;
	      n1->partner = n2;

	      printf( "installed back connection to: %s %d\n", n2->server_name, n2->server_port );
	      printf( "listening on localhost on port: %d\n", i );

	      send_info( s_server, n1, i );
	    }

	  return 1;
	} 	   
      else 
	{
	  return RETURN_CONNECT_ERROR;
	}    
    } 
  else if( node->server_port )
    {
      int s_server = 0;
      int s_accept = 0;

      if( ( ( s_accept = accept_socket( node->s_current ) ) > 0 ) &&		 
	  ( s_server = connect_server( node->server_name, node->server_port ) ) > 0 )
	{
	  struct List*   l = NULL;
	  struct FdNode *n1 = NULL;

	  set_sock_nonblocking( s_accept );
	  set_sock_nonblocking( s_server );
	  
	  l = list_add( fd_list );
	  l->data = n1 = calloc( 1, sizeof( struct FdNode ) );
	  
	  n1->s_read = s_accept;
	  n1->s_write = s_server;
	  n1->parent = node;

	  printf( "incomming back connection\n" );

	  return 1;
	}
      else
	{
	  return RETURN_CONNECT_ERROR;
	}    
    }
  return 0;
}

static void do_cleanup( struct List **fd_list )
{
  struct List *l1;
  int    dirty = 0;

  for( l1 = *fd_list; l1 != NULL; l1 = l1->next )
    {
      struct FdNode *n1 = (struct FdNode*)l1->data;

      if( n1 == NULL )
	continue;

      if( n1->server_port )
	{
	  int found = 0;
	  struct List *l2;

	  for( l2 = *fd_list; l2 != NULL; l2 = l2->next )
	    {
	      struct FdNode *n2 = (struct FdNode*)l2->data;
	      
	      if( n2 == NULL )
		continue;
	      
	      if( n2->partner == n1 )
		{
		  found = 1;
		  break;
		}
	    }

	  if( !found )
	    {
	      /* give up this server even if there is a connection */
	      printf( "removing backserver for: %s\n", n1->server_name );

	      /* close all child connections from this server too */
	      for( l2 = *fd_list; l2 != NULL; l2 = l2->next )
		{
		  struct FdNode *n2 = (struct FdNode*)l2->data;
		  
		  if( n2 == NULL )
		    continue;
	      
		  if( n2->parent == n1 )
		    {
		      clean_up_connection( n2, fd_list );
		      dirty = 1;
		      break;
		    }
		}

	      if( dirty )
		break;

	      clean_up_connection( n1, fd_list );
	    }
	}

      if( dirty )
	l1 = *fd_list;
    }
}

static int time_out_func( struct FdNode* node, void *data )
{
  static time_t atime = 0;
	
  if( atime == 0 )
    {
      atime = time( NULL );
    }
  else 
    {
      time_t ctime = time(NULL);

      if( ctime - atime > 300 )
	{
	  /* remove time outed back server */
	  struct cmp_data *cd = (struct cmp_data*) data;
	
	  atime = ctime;

	  if( cd != NULL )
	    do_cleanup( cd->fd_list );
	}
    }
  return 1;
}

static int proxy_tel_do_read( struct FdNode* node, void *data )
{

  int ret = reader_writer_read( node );
  
  return ret;
}

static int proxy_tel_do_write( struct FdNode* node, void *data )
{
  int ret = reader_writer_write( node );
  
  return ret;
}

int create_proxy_tel( void )
{
  int            s_listen = 0;
  struct List*   fd_list = NULL;
  struct List*   l = NULL;
  struct FdNode* node = NULL;
  struct ProxyCallbacks cb;
  int            ret = 0;
  struct cmp_data cmp_data;
  
  memset( &cmp_data, 0, sizeof( cmp_data ) );
  memset( &cb, 0, sizeof( cb ) );

  if( ( s_listen = bind_socket( SETUP.u.proxy_tel.listen_port, 1 ) ) <= 0 )
    {
      return RETURN_BIND_ERROR;
    }

  l = list_add( &fd_list );
  l->data = node = calloc( 1, sizeof( struct FdNode ) );

  node->s_read = s_listen;

  cb.cmp_node_read = proxy_tel_cmp_node_read;
  cmp_data.s_listen = s_listen;
  cmp_data.fd_list = &fd_list;
  cb.cmp_node_read_data = &cmp_data;

  cb.do_read = proxy_tel_do_read;
  cb.do_read_data = NULL;

  cb.do_write = proxy_tel_do_write;
  cb.do_write_data = NULL;

  cb.time_out =  time_out_func;
  cb.time_out_data = &cmp_data;

  ret = proxy_func( cb, &fd_list );
    
  free_list( &fd_list );

  close( s_listen );

  return ret;
}
