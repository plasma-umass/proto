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
 * @file   timestamp.h
 * @brief  Fine timing management based on rdtsc.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *  Some description about RDTSC can be seen at www.ccsl.carleton.ca/~jamuir/rdtscpm1.pdf.
 */
#ifdef __TIMESTAMP_H__
#define __TIMESTAMP_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>

// Code is borrowed from http://www.mcs.anl.gov/~kazutomo/rdtsc.html.
#if defined(__i386__)
static inline unsigned long long getTimestamp(void)
{
  unsigned long long int x;
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
  return x;
}
#elif defined(__x86_64__)
static inline unsigned long long getTimestamp(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#endif

unsigned long long getElapsedCycles(unsigned long long start) {
  unsigned long long stop = getTimestamp();
  
  return(stop - start);
}

#endif
