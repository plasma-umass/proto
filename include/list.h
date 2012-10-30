#ifndef __LIST_H__
#define __LIST_H__
/*
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

/*
 * @file   list.h
 * @brief  Link list, in order to avoid the usage of STL. 
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
struct lnode {
  struct lnode * prev;
  struct lnode * next;
};

// Initialize a node
inline void nodeInit(lnode * node) {
  node->next = node->prev = node;
}

inline void listInit(lnode * node) {
  nodeInit(node);
}

// Whether a list is empty
inline bool isListEmpty(lnode * head) {
  return head->next == head;
  //return head->next == head->prev;
}

// Next node of current node
inline lnode * nextEntry(lnode * cur) {
  return cur->next;
}

// Insert one entry to two consequtive entries
inline void __insert_between(lnode * node, lnode * prev, lnode * next) {
  node->next = next;
  node->prev = prev;
  prev->next = node;
  next->prev = node;
}

// Insert one entry to after specified entry prev (prev, prev->next)
inline void listInsertNode(lnode * node, lnode * prev) {
  __insert_between(node, prev, prev->next);
}

// Insert one entry to the tail of specified list.
// Insert between tail and head
inline void listInsertTail(lnode * node, lnode * head) {
  __insert_between(node, head->prev, head);
}

// Insert one entry to the head of specified list.
// Insert between head and head->next
inline void listInsertHead(lnode * node, lnode * head) {
  __insert_between(node, head, head->next);
}

// Internal usage to link p with n
// Never use directly outside.
inline void __list_link(lnode *p, lnode *n)
{   
    p->next = n;
    n->prev = p;
}

// We need to verify this
// Insert one entry to the head of specified list.
// Insert the list between where and where->next
inline void listInsertList(lnode * list, lnode * where) {
  // Insert after where.
  __list_link(where, list);

  // Then modify other pointer
  __list_link(list->prev, where->next);
}

// Insert one list between where->prev and where, however,
// we don't need to insert the node "list" itself
inline void listInsertListTail(lnode * list, lnode * where) {
#if 0
  // Link between where->prev and first node of list.
  lnode* origtail = where->prev;
  lnode* orighead = where;
  lnode* newhead = list->next;
  lnode* newtail = list->prev;

  origtail->next = newhead;
  newhead->prev = origtail;

  newtail->next = orighead;
  orighead->prev = newtail;
    
    p->next = n;
    n->prev = p;
#else
  __list_link(where->prev, list->next);

  // Link between last node of list and where.
  __list_link(list->prev, where);
#endif
}

// Delete an entry and re-initialize it.
inline void listRemoveNode(lnode * node) {
  __list_link(node->prev, node->next);
  nodeInit(node); 
}

// Check whether current node is the tail of a list
inline bool isListTail(lnode *node, lnode *head)
{
    return node->next == head;
}

// Retrieve the first item form a list
// Then this item will be removed from the list.
inline lnode * listRetrieveItem(lnode * list) {
  lnode * first = NULL;

  // Retrieve item when the list is not empty
  if(!isListEmpty(list)) {
    first = list->next;
    listRemoveNode(first);
  }
  
  return first;
}

// Retrieve all items from a list and re-initialize all source list.
inline void listRetrieveAllItems(lnode * dest, lnode * src)
{
  lnode * first, * last;
  first = src->next;
  last  = src->prev;

  first->prev = dest;
  last->next = dest;
  dest->next = first;
  dest->prev = last;

  // reinitialize the source list
  listInit(src);
}

// Print all entries in the link list
inline void listPrintItems(lnode * head, int num) {
  int i = 0;
  lnode * first, * entry;
  entry = head->next;
  first = head;

 while(entry != first && i < num) {
    fprintf(stderr, "%d: ENTRY %d: %p (prev %p). HEAD %p\n", getpid(), i++, entry, entry->prev, head);
    entry = entry->next;
  }
}

/* Get the pointer to the struct this entry is part of
 *
 */
#define container_of(ptr, type, member) ({ \
                const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                (type *)( (char *)__mptr - offsetof(type,member) );})


};
#endif /* __ALIVETHREADS_H__ */
