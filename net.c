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
#include "net.h"
#include "utils.h"
#include "xhopper2.h"
#include "list.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/time.h>

/* According to POSIX 1003.1-2001 */
#include <sys/select.h>

#ifdef HPUX
void herror(char *s);
#endif

extern int errno;

static struct List* what_todo_list_read_write( struct List *fd_s, int read );

int connect_server( const char* host, int port )
{
    int                sockfd;
    struct sockaddr_in addresse;
    struct hostent     *rechner = NULL;

    debug( "looking up host\n" );
    
    rechner = gethostbyname( host );

    if( rechner == NULL ) {
	herror( "cannot lookup host" );
	return -1;
    }

    memset( &addresse, 0, sizeof( addresse ) );

    addresse.sin_family = AF_INET;
    addresse.sin_port = htons(port);
    memcpy( &addresse.sin_addr, rechner->h_addr_list[0], sizeof( addresse.sin_addr ) );

    debug( "creating socket\n" );

    if( (sockfd = socket( PF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
	perror( "creating socket failed" );
	return -2;
    }
    
    debug( "connecting host\n" );

    if( connect( sockfd, ( struct sockaddr* ) &addresse, sizeof( addresse ) ) ) {
	perror( "connect failed" );
	close( sockfd );
	return -3;
    }

    return sockfd;   
}

int bind_socket( int port, int nonblocking )
{
  int                sockfd = 0;
  int                i = 0;
  struct sockaddr_in addresse;
  
  if( (sockfd = socket( PF_INET, SOCK_STREAM, 0 ) ) < 0 ) 
    {
      perror( "creating socket failed" );
      return -1;
    }
      
  i = 1;

#ifndef WIN32
  /* On WIN32 with SO_REUSEADDR two processes can listen on 
     the same port at the same time. That's a different behavoir
     then on UNIX like systems.
     If we would use this option we would always install back 
     connections on port 6000 since xhopper simply tries out,
     which port can be used and does lookup which ports already
     in use by the app itself.
  */
  setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof( i ) );
#endif      

  if( nonblocking == 1 )
    set_sock_nonblocking( sockfd );

  memset( &addresse, 0, sizeof( addresse ) );
      
  addresse.sin_family = AF_INET;
  addresse.sin_port = htons(port);
  memset( &addresse.sin_addr, 0, sizeof( addresse.sin_addr ) );
      
  if( bind( sockfd, (struct sockaddr *) &addresse, sizeof( addresse ) ) ) 
    {
      perror( "bind failed" );
      close( sockfd );
      return -2;
    }
      
  printf( "listening on localhost on port %d\n", port);
      
  if( listen( sockfd, 128 ) ) 
    {
      perror( "listen failed" );
      close( sockfd );
      return -3;
    }

  return sockfd;
}   

int accept_socket( int sockfd )
{
  return accept_socket2( sockfd, NULL );
}

int accept_socket2( int sockfd, char *ip_buffer )
{
    struct sockaddr_in addresse;
    SIZE_T  adrlaenge = sizeof( struct sockaddr_in );
    int     connfd;

    if( sockfd <= 0 )
      return -1;

SIGREACHED:

    if( (connfd = accept( sockfd, (struct sockaddr *) &addresse, &adrlaenge )) <= 0 )
      {
	printf( "connection not established\n" );	
	if( errno == EINTR )
	{
		printf( "sig reached while accepting from socket\n" );
		goto SIGREACHED;
	}
	return -2;
      } else {
	printf( "connection established from: %s\n", inet_ntoa(addresse.sin_addr) );	

	if( ip_buffer != NULL )
	  {
	    strncpy( ip_buffer, inet_ntoa(addresse.sin_addr), SERVER_BUF_LEN );
	  }
      }

    return connfd;
}

void set_sock_nonblocking( int fd )
{ 
  int opts = fcntl( fd, F_GETFL );

  if( opts < 0 )
    {
      perror( "fcntl(F_GETFL)" );
      exit( RETURN_ERROR );
    }

  opts = (opts | O_NONBLOCK );

  if( fcntl( fd, F_SETFL, opts ) < 0 )
    {
      perror( "fcntl(F_SETFL)" );
      exit( RETURN_ERROR );
    } 
}

static struct List* what_todo_list_read_write( struct List *fd_s, int read )
{
  fd_set  list;
  int     max_fd = 0;
  int     ret = 0;
  struct List *l = fd_s;
  struct List *rlist = NULL;
  struct timeval tv;
  int    count = 0;

    FD_ZERO( &list );

    for( l = fd_s, count = 0; l != NULL; l = l->next, count++ )
      {
	struct FdNode *node = (struct FdNode*)l->data;

	if( count > FD_SETSIZE )
	  {
	    printf( "ERROR: Maximum Number of connections reached!\n" );
	    break;
	  }

	if( max_fd < node->s_read )
	  max_fd = node->s_read;

	if( max_fd < node->s_write )
	  max_fd = node->s_write;

	if( node->s_write )
	    FD_SET( node->s_write, &list );

	if( node->s_read )
	    FD_SET( node->s_read, &list );
      }

    tv.tv_sec = 0;
    tv.tv_usec = 5000;

    if( read == 0 )
      {
	ret = select( max_fd + 1, &list, NULL, NULL, &tv );
      } 
    else 
      {
	ret = select( max_fd + 1, NULL, &list, NULL, &tv );
      }

    if( ret > 0 )
      {
	rlist = NULL;

	for( l = fd_s; l != NULL; l = l->next )
	  {
	    struct List *ll = NULL;
	    struct FdNode *node = (struct FdNode*)l->data;
	    
	    node->s_current = 0;

	    if( FD_ISSET( node->s_read, &list ) && FD_ISSET( node->s_read, &list ) )
	      {
		node->s_current = -1;
		ll = list_add( &rlist );
		ll->data = node;
	      }
	    else if( FD_ISSET( node->s_read, &list ) )
	      {
		node->s_current = node->s_read;
		ll = list_add( &rlist );
		ll->data = node;
	      }
	    else if( FD_ISSET( node->s_write, &list ) )
	      {
		node->s_current = node->s_write;
		ll = list_add( &rlist );
		ll->data = node;
	      }
	  }
      }

    return rlist;
}

struct List* what_todo_list_read( struct List *fd_s )
{
  return what_todo_list_read_write( fd_s, 0 );
}

struct List* what_todo_list_write( struct List *fd_s )
{
  return what_todo_list_read_write( fd_s, 1 );
}
