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
#include "proxy.h"
#include "utils.h"
#include "net.h"
#include "list.h"
#include "xhopper2.h"
#include "compress.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>


void free_list( struct List** list )
{
  struct List* l;

  if( list == NULL )
    return;

  for( l = *list; l != NULL; l = l->next )
    {
      free( l->data );
    }

  list_free( list );
}

struct cmp_data
{
  int s_listen;
  struct List **fd_list;
};

int clean_up_connection( struct FdNode *node, struct List** fd_list )
{
  struct List *l = NULL;
  
  for( l = *fd_list; l != NULL; l = l->next )
    {
      if( l->data == node )
	{
	  if( node->s_read )
	    close( node->s_read );

	  if( node->s_write )
	    close( node->s_write );
	  
	  if( node->send_queue_reader != NULL )
	    {
	      free_list( &node->send_queue_reader );
	    }
	  
	  if( node->send_queue_writer != NULL )
	    {
	      free_list( &node->send_queue_writer );
	    }
	  
	  print_stats( 0 );
	  printf( "connection closed\n" );

	  free( node );
	  list_remove( &l );
	  return 1;
	}
    }  
  return 0;
}

static int proxy_cmp_node_read( struct FdNode *node, void *data )
{
  int s_listen = ((struct cmp_data*)data)->s_listen;
  struct List **fd_list = ((struct cmp_data*)data)->fd_list;

  if( node->s_current == s_listen )
    {
      int s_server = 0;
      int s_accept = 0;
      
      if( ( ( s_accept = accept_socket( s_listen ) ) > 0 ) &&
	  ( s_server = connect_server( SETUP.u.proxy.server_name, SETUP.u.proxy.server_port ) ) > 0 )
	{
	  struct List*   l = NULL;

	  set_sock_nonblocking( s_accept );
	  set_sock_nonblocking( s_server );
	  
	  l = list_add( fd_list );
	  l->data = node = calloc( 1, sizeof( struct FdNode ) );
	  
	  node->s_read = s_accept;
	  node->s_write = s_server;		     
	  return 1;
	}  
      else 
	{
	  return RETURN_CONNECT_ERROR;
	}
    }
  return 0;
}

static int proxy_do_read( struct FdNode* node, void *data )
{
  if( SETUP.u.proxy.wait_time > 0 ) 
	{
		usleep( SETUP.u.proxy.wait_time * 1000 );
	}

  return reader_writer_read( node );
}

static int proxy_do_write( struct FdNode* node, void *data )
{
  return reader_writer_write( node );
}

int create_pure_proxy( void )
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

  if( ( s_listen = bind_socket( SETUP.u.proxy.listen_port, 1 ) ) <= 0 )
    {
      return RETURN_BIND_ERROR;
    }

  l = list_add( &fd_list );
  l->data = node = calloc( 1, sizeof( struct FdNode ) );

  node->s_read = s_listen;

  cb.cmp_node_read = proxy_cmp_node_read;
  cmp_data.s_listen = s_listen;
  cmp_data.fd_list = &fd_list;
  cb.cmp_node_read_data = &cmp_data;

  cb.do_read = proxy_do_read;
  cb.do_read_data = NULL;

  cb.do_write = proxy_do_write;
  cb.do_write_data = NULL;

  ret = proxy_func( cb, &fd_list );
    
  free_list( &fd_list );

  close( s_listen );

  return ret;
}

static int _reader_writer_read( int fd, struct SendNode *node )
{
  int len;

  if( ( len = (int) read( fd, node->buffer, BUFFER_READ_SIZE ) ) > 0 )
    {
      node->len = len;     

      SETUP.stat_read_data += len;

/*		fwrite( node->buffer, len, 1, stdout );
		printf( "======= END READ =======\n" );
*/

      return RETURN_CONTINUE;
    } 
  else if( len == -1 && errno == EAGAIN )
    {
      node->len = 0;
      return RETURN_CONTINUE;
    }

  return RETURN_ERROR;
}

int reader_writer_read( struct FdNode *node )
{
  struct List**   queue = NULL;
  struct List*    sq = NULL; 
  struct SendNode *sn = NULL; 
  int             fd = 0;
  int             ret = 0;
  static struct SendNode *buffer_node = NULL;
  int             used_buffer = 0;

  fd = node->s_current;

  if( node->s_current == node->s_read )
    {
      queue = &node->send_queue_writer;      
    }
  else
    {
      queue = &node->send_queue_reader;      
    }

  if( *queue != NULL )
    {
      struct List* last = list_get_last( queue );	  

      if( last != NULL && last->data != NULL && ((struct SendNode*)(last->data))->len == 0 )
	{
	  sn = (struct SendNode*)last->data;
	}
    }

  if( buffer_node == NULL )
    {
      buffer_node = calloc( 1, sizeof( struct SendNode ) );
      buffer_node->pcurrent = buffer_node->buffer;
    }

  if( sn == NULL )
    {
      sn = buffer_node;
      sn->pcurrent = sn->buffer;
      used_buffer = 1;
    }

  ret = _reader_writer_read( fd, sn );
 
  if( used_buffer == 1 && sn->len > 0 )
    {
      sq = list_append( queue );
      sq->data = buffer_node;
      buffer_node = NULL;
    }

  if( ret == RETURN_CONTINUE && sn->len > 0 )
    {
      if( SETUP.mode & MODE_COMPRESS )
	{
	  if( node->s_current == node->s_read )
	    {
	      do_compress( sn->buffer, &sn->len );
	      debug_header( sn->buffer );
	    } 
	  else 
	    {
	      int cont = 0;

	      do 
		{
		  debug_header( sn->buffer );
		  cont = do_uncompress( sn->buffer, &sn->len, &node->root_reader );

		  if( cont > 0 )
		    {
		      sq = list_append( queue );
		      sn = calloc( 1, sizeof( struct SendNode ) );
		      
		      sq->data = sn;
		      sn->pcurrent = sn->buffer;		      
		    }

		} while ( cont > 0 );
	    }
	}
      else if( SETUP.mode & MODE_UNCOMPRESS )
	{
	  if( node->s_current == node->s_read )
	    {
	      int cont = 0;

	      do 
		{
		  debug_header( sn->buffer );

		  if( GET_HEADER_FLAG( sn->buffer, FLAG_REQUEST ) )
		    {
		      free( sn );
		      list_remove( &sq );

		      if( sq == NULL )
			*queue = NULL;

		      return RETURN_GOT_REQUEST;
		    }

		  cont = do_uncompress( sn->buffer, &sn->len, &node->root_writer );

		  if( cont > 0 )
		    {
		      sq = list_append( queue );
		      sn = calloc( 1, sizeof( struct SendNode ) );
		      
		      sq->data = sn;
		      sn->pcurrent = sn->buffer;		      
		    }

		} while ( cont > 0 );
	    } 
	  else 
	    {
	      do_compress( sn->buffer, &sn->len );
	      debug_header( sn->buffer );
	    }	  
	}	
    }
 
  return ret;
}

int reader_writer_write( struct FdNode *node )
{
  struct SendNode *sn = NULL;
  struct List     **sq = NULL;
  int             len = 0;
  int             fd = 0;
    
  fd = node->s_current;

  if( node->s_current == node->s_read )
    {
      sq = &node->send_queue_reader;
    }
  else
    {
      sq = &node->send_queue_writer;
    }

  if( *sq == NULL || (*sq)->data == NULL )
    return RETURN_CONTINUE;

  sn = (struct SendNode*)(*sq)->data;

  for(;;)
    {
      if( sn->len == 0 )
	{
	  free( sn );
	  list_remove( sq );
	  return RETURN_CONTINUE;
	}

      if( ( len = write( fd, sn->pcurrent, sn->len ) ) > 0 )
	{
	  sn->len -= len;
	  sn->pcurrent += len;
	  
	  SETUP.stat_write_data += len;

	  if( sn->len == 0 )
	    {
	      free( sn );
	      list_remove( sq );
	      
	      if( *sq != NULL && (*sq)->data != NULL )
		{
		  sn = (struct SendNode*)(*sq)->data;
		  debug( "continue: sn->len = %d\n", sn->len );
		  continue;
		}	      
	    }
	  return RETURN_CONTINUE;
	}
      else if( len == -1 && errno == EAGAIN )
	{
	  return RETURN_CONTINUE;
	}
      else
	{
	  printf( "writing failed\n" );
	  return RETURN_ERROR;
	}
    }

  return 0;
}

int proxy_func( struct ProxyCallbacks cb, struct List** fd_list )
{
   for(;;)
    {
      struct List*   nlist = NULL;
      int            something_todo = 0;
      struct FdNode* node = NULL;
      int            status;

      waitpid( 0, &status , WNOHANG );
      if( cb.time_out != NULL )
	{
	  cb.time_out( NULL, cb.time_out_data );
	}

      if( (nlist = what_todo_list_read( *fd_list ) ) != NULL )
	{
	  struct List* nl = NULL;
	  int next_fd = 0;
	  int closed = 0;

	  for( nl = nlist; nl != NULL; nl = nl->next )
	    {
	      node = (struct FdNode*) nl->data;
	      closed = 0;

	      do
		{
		  if( node->s_current == -1 ) /* both fd's are ready */
		    {
		      next_fd = node->s_write;
		      node->s_current = node->s_read;
		    } 
		  else if ( next_fd != 0 )
		    {
		      node->s_current = next_fd;
		      next_fd = 0;
		    }
		  if( cb.cmp_node_read != NULL && cb.cmp_node_read( node, cb.cmp_node_read_data ) )
		    {
		      ;
		    }
		  else
		    {
		      int r;
		      if( cb.do_read != NULL && 
			  ( r = cb.do_read( node, cb.do_read_data ) ) != RETURN_CONTINUE )
			{
			  if( r == RETURN_CONT_DIRTY )
			    closed = 1;
			  else if( clean_up_connection( node, fd_list ) )
			    {
			      closed = 1;
			      break;
			    }
			} 
		      else if( ((struct FdNode*)(nl->data))->send_queue_reader != NULL ||
			       ((struct FdNode*)(nl->data))->send_queue_writer != NULL )
			{
			  something_todo = 1;
			}
		    }

		} while( next_fd != 0 && closed == 0 );
     
	      if( closed == 1 )
		break;

	    } /* for */

	  list_free( &nlist );

	  if( closed == 1 )
	    continue;
	} /* if */

      /* do we have something to write? */
      if( something_todo == 0 )
	{
	  struct List *l = NULL;

	  for( l = *fd_list; l != NULL; l = l->next )
	    {
	      if( ((struct FdNode*)(l->data))->send_queue_reader != NULL ||
		  ((struct FdNode*)(l->data))->send_queue_writer != NULL )
		{
		  something_todo = 1;
		  break;
		}
	    }
	  if( something_todo == 0 )
	    continue;
	}

      if( (nlist = what_todo_list_write( *fd_list ) ) != NULL )
	{
	  struct List* nl = NULL;
	  int          next_fd = 0;

	  for( nl = nlist; nl != NULL; nl = nl->next )
	    {
	      int closed = 0;
	      node = (struct FdNode*) nl->data;

	      do
		{
		  int r;

		  if( node->s_current == -1 ) /* both fd's are ready */
		    {
		      next_fd = node->s_write;
		      node->s_current = node->s_read;
		    } 
		  else if ( next_fd != 0 )
		    {
		      node->s_current = next_fd;
		      next_fd = 0;
		    }
		  
		  if( cb.do_write != NULL && (r = cb.do_write( node, cb.do_write_data )) != RETURN_CONTINUE )
		    {
		      if( r == RETURN_CONT_DIRTY )
			closed = 1;
		      else if( clean_up_connection( node, fd_list ) )
			{
			  closed = 1;
			  break;
			}
		    }	 
		} while( next_fd != 0 && closed == 0 );
     
	      if( closed == 1 )
		break;

	    } /* for */

	  list_free( &nlist );
	} /* if */
    } 
	return 0;
}

void print_stats( int fd )
{
  char buffer[1024];

  sprintf( buffer, "in: %luB %luK %luM out:%luB %luK %luM\n",
	   SETUP.stat_read_data, 
	   SETUP.stat_read_data / 1024, 
	   SETUP.stat_read_data / 1024 / 1024, 
	   SETUP.stat_write_data,
	   SETUP.stat_write_data / 1024,
	   SETUP.stat_write_data / 1024 / 1024 );
  write( fd, buffer, strlen( buffer ) );
}
