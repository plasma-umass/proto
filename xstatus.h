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
 * @file:   xstatus.h
 * @brief:  Definition of types which will be used by multiple classes.
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XSTATUS_H_
#define _XSTATUS_H_


typedef enum {
  THREAD_STATUS_INITIAL,
  THREAD_STATUS_RUNNING,
  THREAD_STATUS_COND_WAITING,
  THREAD_STATUS_LOCK_WAITING,
  THREAD_STATUS_BARRIER_WAITING,
  THREAD_STATUS_SIGNAL_HANDLING,
  THREAD_STATUS_JOINING, // Join the children threads
  THREAD_STATUS_DEAD       // Thread already exit but not cleaned up.
} e_thread_status;


#endif /* _XSTATUS_H_ */
