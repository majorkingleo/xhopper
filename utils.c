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
#include "utils.h"
#include "xhopper2.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

char* get_file_name( char *file )
{
  return get_suffix( file, '/' );
}

void debug( char *fmt, ... )
{
  va_list ap;

  if( SETUP.mode & MODE_DEBUG )
    {
      va_start(ap, fmt);

      vprintf( fmt, ap );

      va_end(ap);
    }
}

int is_int( char *buf )
{
  int i, len = strlen( buf );

  for( i = 0; i < len; i++ )
    {
      if( isdigit( buf[i] ) == 0 )
	return 0;
    }

  return 1;
}

char *get_suffix( char *file, char c )
{
  int count = strlen( file );
  int i;

  for( i = count - 1; i >= 0; i-- )
    {
      if( file[i] == c && i < count - 1 )
	return &file[i+1];
    }

  return file;  
}

int icase_cmp( char *a, char *b )
{
  char *aa = strdup( a );
  char *bb = strdup( b );
  unsigned i = 0;
  unsigned len = 0;
  int ret;

  for( i = 0, len = strlen( aa ); i < len; i++ )
    aa[i] = toupper( aa[i] );

  for( i = 0, len = strlen( bb ); i < len; i++ )
    bb[i] = toupper( bb[i] );

  ret = strcmp( aa, bb );

  free( aa );
  free( bb );
  
  return ret;
}
