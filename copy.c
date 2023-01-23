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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct Progress
{
  unsigned long size;
  unsigned long current;
  unsigned step;
  unsigned next_step;
};

static void init_progress( struct Progress *p, unsigned size )
{
  p->size = size;
  p->step = size / 70;
  p->next_step = p->step;
  p->current = 0;
}

static void do_progress( struct Progress *p, unsigned len )
{
  if( p->size )
    {
      p->current += len;
	  
      if( len > p->step )
	{
	  int i;
	  
	  for( i = p->step; i < len; i += p->step )
	    {
	      printf( "=" );
	    }
	  
	  fflush( stdout );	
	}
      else if( p->current >= p->next_step )
	{
	  while( p->current >= p->next_step )
		{
			printf( "." );
	  		fflush( stdout );	      
	  		p->next_step += p->step;
		}
	}

      if( p->current >= p->size )
	printf( "\n" );
    }  
}

static int create_file_server2( void );

int create_file_server( void )
{
  int ret;

  do
    {
      ret = create_file_server2();
    } while( ret == 0 );

  return ret;
}

static int create_file_server2( void )
{
  int   s_listen = 0;
  int   s_accept = 0;
  struct Node* root = NULL;
  struct Progress p;

  if( ( s_listen = bind_socket( SETUP.u.copy_server.listen_port, 0 ) ) <= 0 )
    {
      return RETURN_BIND_ERROR;
    }

  if( ( s_accept = accept_socket( s_listen ) ) >= 0 )
    {
      char buffer[BUFFER_LEN];
      int len = 0;
      char *file_name = NULL;
      int file = 0;

      while( ( len = read( s_accept, buffer, BUFFER_READ_SIZE + HEADER_SIZE ) ) >= 0 )
	{
	  int cont = 0;

	  if( len )
	    {
	      debug( "==== len: %d\n", len );
	      debug_header( buffer );
	    }

	  if( file_name == NULL )
	    {
	      if( queue( buffer, &len, &root ) != 0 )
		{
		  debug_header( buffer );

		  if( GET_HEADER_FLAG( buffer, FLAG_FILENAME ) )
		    {
		      char *buf = NULL;
		      unsigned flen = 0;
		      unsigned long size = 0;

		      buf = malloc( GET_HEADER_LEN( buffer ) + 1 );
		      memcpy( buf, buffer + HEADER_SIZE, GET_HEADER_LEN( buffer ) );
		      buf[GET_HEADER_LEN(buffer)] = '\0';

		      flen = strlen( get_file_name( buf ) );
		      file_name = malloc(  flen + 1 );
		      memcpy( file_name, get_file_name( buf ), flen );
		      file_name[flen] = '\0';
		      
		      sscanf( buf, "%lu", &size );

		      init_progress( &p, size );

		      if( (file = open( file_name, O_CREAT | O_TRUNC | O_RDWR, 0666 )) == -1 )
			{
			  printf( "cannot create file: '%s'\n", file_name );
			  free( file_name );
			  file_name = NULL;
			  break;
			}

		      printf( "opened file: '%s'\n", file_name );
		      continue;
		    }
		} else {
		  continue;
		}
	    } else {
	      do {
		cont = do_uncompress( buffer, &len, &root );

		do_progress( &p, len );

		write( file, buffer, len );
		len = 0;
	      } while( cont > 0 );
	    }

	  if( cont == -2 )
	    break;
	}

      if( file )       
	{
	  fsync( file );
	  close( file );     
	  
	  if( file_name )
	    {
	      printf( "got file: %s\n", file_name );
	      free( file_name );
	      file_name = NULL;
	    }
	}

      close( s_accept );
   }	  

  close( s_listen );

  return 0;
}

int copy_file( void )
{
  int  file = 0;
  int  s_server = 0;
  char buffer[BUFFER_SIZE];
  char buf2[1024];
  int  len = 0;
  int  len2 = 0;
  struct stat buf;
  struct Progress p;
  char *ext;

  if( ( file = open( SETUP.u.copy_client.file_name, 0 ) ) == -1 )
    {
      printf( "cannot open file: %s\n", SETUP.u.copy_client.file_name );
      return RETURN_CANNOT_OPEN_FILE;
    }

  if( ( s_server = connect_server( SETUP.u.copy_client.server_name, 
				   SETUP.u.copy_client.server_port ) ) <= 0 )
    {
      return RETURN_CONNECT_ERROR;
    }

  if( stat( SETUP.u.copy_client.file_name, &buf ) == 0 )
    {
      init_progress( &p, buf.st_size );
    }
  
  ext = get_suffix( SETUP.u.copy_client.file_name, '.' );

  if( icase_cmp( ext, "gz" ) ||
      icase_cmp( ext, "bz" ) ||
      icase_cmp( ext, "bz2" ) ||
      icase_cmp( ext, "Z" )   ||
      icase_cmp( ext, "zip" ) ||
      icase_cmp( ext, "mp3" ) ||
      icase_cmp( ext, "ogg" ) ||
      icase_cmp( ext, "avi" ) ||
      icase_cmp( ext, "mpg" ) ||
      icase_cmp( ext, "mpeg" ) )
    {
      /* do not compress already compressed files */
      SETUP.mode ^= MODE_COMPRESS;
    }

  /* send filename */

  memset( buffer, 0, HEADER_SIZE );
  
  sprintf( buf2, "%lu/", buf.st_size );

  len = strlen( get_file_name(SETUP.u.copy_client.file_name) );
  len2 = strlen( buf2 );
  SET_HEADER_LEN( buffer, len + len2 );
  SET_HEADER_FLAG( buffer, FLAG_FILENAME );
  memcpy( buffer + HEADER_SIZE, buf2, len2 );
  memcpy( buffer + HEADER_SIZE + len2, get_file_name(SETUP.u.copy_client.file_name), len );
  len += HEADER_SIZE;
  len += len2;

  debug_header( buffer );

  write( s_server, buffer, len );

  printf( "sending file: %s\n", SETUP.u.copy_client.file_name );

  while( ( len = read( file, buffer, BUFFER_READ_SIZE ) ) > 0 )
    {
      int l;
      char *puf;

      do_progress( &p, len );

      do_compress( buffer, &len );

      debug_header( buffer );

      puf = buffer;

      do {
	l = write( s_server, puf, len );

	if( l == len )
	  break;

	puf += l;
	len -= l;

      } while( l > 0 );
    }

  memset( buffer, 0, HEADER_SIZE );
  SET_HEADER_LEN( buffer, 1 );
  SET_HEADER_FLAG( buffer, FLAG_FILEEND );
  write( s_server, buffer, HEADER_SIZE + 1 );

  debug_header( buffer );

  close( s_server );
  close( file );

  printf( "done\n" );

  return RETURN_SUCCESS;
}
