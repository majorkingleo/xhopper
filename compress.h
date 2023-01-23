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
#ifndef XHOPPER_COMPRESS_H
#define XHOPPER_COMPRESS_H

#define HEADER_LEN_LEN sizeof(int)
#define HEADER_FLAG_LEN sizeof(int)

#define HEADER_SIZE (HEADER_LEN_LEN + HEADER_FLAG_LEN)

#define GET_HEADER_FLAG( x, flag ) (GET_HEADER_FLAGS( x ) & flag)
#define SET_HEADER_FLAG( x, flag ) SET_HEADER_FLAGS( x, GET_HEADER_FLAGS( x ) | flag )
#define DEL_HEADER_FLAG( x, flag ) \
    (GET_HEADER_FLAG( x, flag ) ? SET_HEADER_FLAGS( x, GET_HEADER_FLAGS( x ) ^ flag ) :  GET_HEADER_FLAG( x, flag ))

#define FLAG_INIT     1

#define FLAG_COMP       FLAG_INIT
#define FLAG_DIE      ( FLAG_INIT << 1 )
#define FLAG_FILENAME ( FLAG_INIT << 2 )
#define FLAG_REQUEST  ( FLAG_INIT << 3 )
#define FLAG_FILEEND  ( FLAG_INIT << 4 )

struct Node
{
    struct Node *next;
    int         len;
    int         plen;
    char        *buffer;
    int         header_len;
};

int GET_HEADER_LEN( char *x );
void SET_HEADER_LEN( char *x, int y );
int GET_HEADER_FLAGS( char *x );
void SET_HEADER_FLAGS( char *x, int y );

void debug_header2( char *file, int line, char *buffer ); 
#define debug_header( buf ) debug_header2( __FILE__, __LINE__, buf )
void do_compress( char *buffer, int *len );
int do_uncompress( char *buffer, int *len, struct Node** root );
int queue( char *buffer,int *len, struct Node **root );

#endif
