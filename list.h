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
#ifndef XHOPPER_LIST_H
#define XHOPPER_LIST_H

struct List
{
    struct List *next;
    struct List *prev;
    
    void *data;
};

void list_free( struct List** list );

struct List* list_add( struct List** list );
struct List* list_append( struct List** list ); /* add at the end of the list */
struct List* list_insert( struct List** list );
void list_remove( struct List** list );
int list_count( struct List** list );
struct List* list_get_last( struct List** list );

#endif
