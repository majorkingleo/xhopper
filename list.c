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
#include "list.h"
#include <stdio.h>
#include <stdlib.h>

void list_free( struct List** list )
{
    if( list == NULL || *list == NULL )
	return;

    list_free( &(*list)->next );

    if( (*list)->next == NULL )
    {
	free( *list );
	*list = NULL;
	return;
    }
}

struct List* list_add( struct List** list )
{
    struct List *l = NULL;

    if( list == NULL )
	return NULL;
    
    l =  calloc( 1, sizeof( struct List ) );
    
    if( *list == NULL )
    {
	*list = l;
	return l;
    }

    l->prev = *list;
    l->next = (*list)->next;
    (*list)->next = l;

    if( l->next )
	l->next->prev = l;

    return l;
}

struct List* list_insert( struct List** list )
{
    struct List *l = NULL;

    if( list == NULL )
	return NULL;
  
    l =  calloc( 1, sizeof( struct List ) );

    if( *list == NULL )
    {
	*list = l;
	return l;
    }

    l->next = *list;
    l->prev = (*list)->prev;

    if( (*list)->prev )      
      (*list)->prev->next = l;

    (*list)->prev = l;
    
    return l;
}

void list_remove( struct List** list )
{
    struct List* l = NULL;

    if( list == NULL || *list == NULL )
	return;

    l = *list;

    if( l->prev )
	l->prev->next = l->next;

    if( l->next )
	l->next->prev = l->prev;

    if( l->prev )
	*list = l->prev;
    else if( l->next )
	*list = l->next;
    else
	*list = NULL;

    free( l );
}

int list_count( struct List** list )
{
  int count = 0;
  struct List *l = NULL;

  if( list == NULL || *list == NULL )
    return 0;

  l = *list;

  while( l )
    {
      count++;
      l = l->next;
    }

  return count;
}

struct List* list_append( struct List** list )
{
  struct List* l;

  if( list == NULL )
    return NULL;

  l = *list;

  if( l == NULL )
    {
      l = list_add( &l );
      *list = l;
      return l;
    }

  for( l = *list; l->next != NULL; l = l->next )
    ;

  return list_add( &l );  
}

struct List* list_get_last( struct List** list )
{
  struct List* l;
  
  if( list == NULL || *list == NULL )
    return NULL;
  
  for( l = *list; l->next != NULL; l = l->next )
    ;
  
  return l;
}
