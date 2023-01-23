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
#ifndef XHOPPER_PROXY_H
#define XHOPPER_PROXY_H

#include "xhopper2.h"

struct SendNode
{
  char buffer[BUFFER_LEN];
  char *pcurrent;
  int  len;
};

int create_pure_proxy( void );
int reader_writer_read( struct FdNode *node );
int reader_writer_write( struct FdNode *node );
void free_list( struct List** list );
int clean_up_connection( struct FdNode *node, struct List** fd_list );
void print_stats( int fd );

typedef int node_func( struct FdNode* node, void *data );

struct ProxyCallbacks
{
  node_func *cmp_node_read;
  node_func *do_read;
  node_func *do_write; 
  node_func *time_out; 

  void * do_write_data;
  void * cmp_node_read_data;
  void * do_read_data;
  void * time_out_data;
};

int proxy_func( struct ProxyCallbacks cb, struct List** fd_list );

#endif
