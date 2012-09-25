// -*- C++ -*-
/*
  Copyright (c) 2012-2013, University of Massachusetts Amherst.

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
 * @file:   xevent.h
 * @brief:  
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XEVENT_H_
#define _XEVENT_H_

#include "list.h"
  
typedef enum {
  E_EVENT_YIELD = 0,
  E_EVENT_RELEASE_LOCK,
} e_event_type;

class xevent {

public:

  xevent() {
    listInit(&toqueue);
  }


  lnode  toqueue; // event pointer.
  e_event_type type;
  union {
    struct { xthread * thread; xqueue * queue; } YIELD;
    struct { spinlock * lock; } RELEASELOCK;
  } args;
};

#endif /* _ */
