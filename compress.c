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
#include "compress.h"
#include "xhopper2.h"
#include "utils.h"

#include "zlib/zlib.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <netdb.h>
#include <netinet/in.h>


static void comp_error( char *message, int status );
static struct Node* append_node( struct Node* node, char *buffer, int len );

int GET_HEADER_LEN( char *x )
{
	int i;
	memcpy( &i, x, sizeof( int ) );
	return ntohl( i );
}

void SET_HEADER_LEN( char *x, int y )
{
	int i;
	i = htonl( y );
	memcpy( x, &i, sizeof( int ));
}

int GET_HEADER_FLAGS( char *x )
{
	int i;
	memcpy( &i, x + HEADER_LEN_LEN, sizeof( int ) );
	return ntohl( i );
}

void SET_HEADER_FLAGS( char *x, int y )
{
	int i;
	i = htonl( y );
	memcpy( x + HEADER_LEN_LEN, &i, sizeof( int ));
}

static void comp_error( char *message, int status )
{
    char *error = "UNKNOWN";
    
    switch( status )
    {
	case Z_OK:         error = "Z_OK"; break;
	case Z_MEM_ERROR:  error = "Z_MEM_ERROR"; break;
	case Z_BUF_ERROR:  error = "Z_BUF_ERROR"; break;
	case Z_DATA_ERROR: error = "Z_DATA_ERROR"; break;
    }

    printf( "%s ret: %d => %s\n", message, status, error );   
    exit(5);
}

void do_compress( char *buffer, int *len )
{
    char  buf[BUFFER_SIZE];
    uLong buf_len = BUFFER_SIZE - HEADER_SIZE;
    int   ret;
    static int all_in = 0;
    static int all_out = 0;
    static int count = 0;    
    static int fav = 0;
    static int sav = 0;
    static int misses = 0;
    static int autoskip = 0;
    static int autoover = 0;

    memset( buf, 0, HEADER_SIZE );

    if( *len < 20 || ( ( SETUP.mode & MODE_FILETRANSFER ) && !(SETUP.mode & MODE_COMPRESS ) ) )
    {
	autoskip += *len;
	autoover += HEADER_SIZE;

	/* do not compress */
	memcpy( buf + HEADER_SIZE, buffer, *len );
	SET_HEADER_LEN( buf, *len );
	*len += HEADER_SIZE;
	memcpy( buffer, buf, *len );
	return;
    }


    if( (ret = compress( buf + HEADER_SIZE, &buf_len, buffer, *len ) ) != Z_OK ) {
	comp_error( "compression failed", ret );
    }

    if( buf_len >= *len )
    {
	/* do not compress */
	memcpy( buf + HEADER_SIZE, buffer, *len );
	SET_HEADER_LEN( buf, *len );
	*len += HEADER_SIZE;
	memcpy( buffer, buf, *len );
	misses++;
	return;
    }

    if( *len - HEADER_SIZE < buf_len )
    {		
	fav += *len - HEADER_SIZE;
    } else {
	sav += *len - HEADER_SIZE;
    }

    all_in  += *len - HEADER_SIZE;
    all_out += buf_len;

    SET_HEADER_LEN( buf, buf_len );
    SET_HEADER_FLAG( buf, FLAG_COMP );

    memcpy( buffer, buf, buf_len + HEADER_SIZE);

    if( count++ > 100 && !(SETUP.mode & MODE_FILETRANSFER) ) 
      {
	printf( "avarage compression: %d%% input %dk output %dk "
		"fsize %d\n\tssize %d misses %d skip %dk skip overhead %dk\n", 
		100 - all_out * 100 / all_in, all_in / 1024, all_out / 1024, 
		fav / count, sav / count, misses,
		autoskip / 1024, autoover / 1024);
	count = 0;
	all_in = 0;
	all_out = 0;
	fav = 0;
	sav = 0;
	misses = 0;
	autoskip = 0;
	autoover = 0;
    }   

    *len = (int)buf_len + HEADER_SIZE;
}


static struct Node* append_node( struct Node* node, char *buffer, int len )
{
    int mlen;

    if( len == 0 )
	return node;

    if( node == NULL)
    {
      node = calloc( 1, sizeof( struct Node ) );

	if( len >= HEADER_SIZE )
	{
	    mlen = GET_HEADER_LEN( buffer );
	    node->buffer = malloc( mlen + HEADER_SIZE );
	    node->len = mlen;
	    node->plen = 0;	
	    node->header_len = HEADER_SIZE;
	} else {
	    printf( "got partial header: len %d\n", len );
	    node->header_len = len;
	    node->buffer = malloc( HEADER_SIZE );
	    memcpy( node->buffer, buffer, len );
	    node->plen = len;
	    return node;
	}
    }

    while( node->header_len == HEADER_SIZE && (node->len + HEADER_SIZE == node->plen) )
    {
	if( node->next == NULL )
	{
	  node->next = append_node( node->next, buffer, len );	    
	    return node;
	}	    

	node = node->next;
    }

    /* the current node is not complete */

    if( node->header_len < HEADER_SIZE )
    {
	if( node->header_len + len >= HEADER_SIZE )
	{
	    printf( "completing partial header: len %ld\n", (unsigned long)HEADER_SIZE - node->header_len );
	    memcpy( node->buffer + node->plen, buffer, HEADER_SIZE - node->header_len );
	    node->plen = HEADER_SIZE;
	    node->len = GET_HEADER_LEN( node->buffer );
	    node->buffer = realloc( node->buffer, node->len + HEADER_SIZE );
	    buffer += HEADER_SIZE - node->header_len;
	    len -= HEADER_SIZE - node->header_len;
	    node->header_len = HEADER_SIZE;

	    if( len == 0 )
		return node;

	} else {
	    printf( "header fragment received: len %d\n", len );

	    memcpy( node->buffer + node->plen, buffer, len );
	    node->plen += len;
	    node->header_len += len;
	    return node;
	}
    }

    if( node->len + HEADER_SIZE - node->plen == len )
    {
	memcpy( node->buffer + node->plen, buffer, node->len + HEADER_SIZE - node->plen );
	node->plen = node->len + HEADER_SIZE;
	return node;

    } else if( node->len + HEADER_SIZE - node->plen < len ) {
	int clen = node->len + HEADER_SIZE - node->plen;

	memcpy( node->buffer + node->plen, buffer, clen );
	len = len - clen;
	node->plen += clen;
	node->next = append_node( node->next, buffer + clen, len );
	return node;

    } else {

	memcpy( node->buffer + node->plen, buffer, len );
	node->plen += len;	
	return node;
    }

    return node;
}

int queue( char *buffer,int *len, struct Node **root )
{
  struct Node* node = *root;
  struct Node* next = NULL;
  
  if( node == NULL && *len == 0 )
    return 0;
 
  if( node == NULL ) {	
    int mlen;
    
    if( *len >= HEADER_SIZE )
      {
	mlen = GET_HEADER_LEN( buffer );
	debug_header( buffer );
	
	if( GET_HEADER_FLAG( buffer, FLAG_DIE ) )
	  return -1;
	
	if( mlen + HEADER_SIZE == *len )
	  return 1;
      }
  }
  
  if( (node = append_node( node, buffer, *len ) ) == NULL )
    return 0;    
  
    if( *root == NULL )
      *root = node;
    else 
      node = *root;
      
    if( node->header_len == HEADER_SIZE && (node->len + HEADER_SIZE == node->plen) )
    {
	memcpy( buffer, node->buffer, node->plen );
	*len = node->plen;

	next = node->next;
	free( node->buffer );
	free( node );
	node = next;	
	*root = node;

	return 1;
    } 

    return (node != NULL && node->header_len == HEADER_SIZE && node->plen == node->len + HEADER_SIZE );
}


int do_uncompress( char *buffer, int *len, struct Node **root )
{
    char  buf[BUFFER_SIZE];
    uLong buf_len = BUFFER_SIZE - HEADER_SIZE;
    int   ret;
    int   mlen;

    if( queue( buffer, len, root ) != 0 )
    {
	if( *len == 0 )
	    return 0;
	mlen = GET_HEADER_LEN( buffer );

	debug_header( buffer );
	
	if( GET_HEADER_FLAG( buffer, FLAG_DIE ) )
	    return -1;

	if( GET_HEADER_FLAG( buffer, FLAG_FILEEND ) )
	  {
	    printf( "got fileend request\n" );

	    *len = 0;
	    return -2;
	  }

	if( !(GET_HEADER_FLAG( buffer, FLAG_COMP ) ) )
	{
	    memcpy( buf, buffer + HEADER_SIZE, *len - HEADER_SIZE );
	    memcpy( buffer, buf, *len - HEADER_SIZE );
	    *len -= HEADER_SIZE;
	    return 1;
	}

	if(  mlen != *len - HEADER_SIZE )
	{
	    printf( "mlen: %d, len: %ld\n", mlen, (unsigned long)*len - HEADER_SIZE );
	    comp_error( "invalid length", *len - HEADER_SIZE );
	}
	
	if( (ret = uncompress( buf, &buf_len, buffer + HEADER_SIZE, *len - HEADER_SIZE) ) != Z_OK ) {
	    comp_error( "decompression failed", ret );
	}

	memcpy( buffer, buf, buf_len );
	*len = (int)buf_len;

	return (*root != NULL);
    }

    *len = 0;
    return 0;
}

void debug_header2( char *file, int line, char *buffer )
{
  debug( "%s:%d HEADER:\n", file, line );
  debug( "\tlen: %d\n", GET_HEADER_LEN( buffer ) );
  debug( "\tcomp: %d\n", GET_HEADER_FLAG( buffer, FLAG_COMP ) );
  debug( "\tdie: %d\n", GET_HEADER_FLAG( buffer, FLAG_DIE ) );  
  debug( "\tfilename: %d\n", GET_HEADER_FLAG( buffer, FLAG_FILENAME ) );
  debug( "\tfileend: %d\n", GET_HEADER_FLAG( buffer, FLAG_FILEEND ) );
}
