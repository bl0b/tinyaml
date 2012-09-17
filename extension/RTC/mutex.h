/* TinyaML
 * Copyright (C) 2007 Damien Leroux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __QUICK_AND_DIRTY_MUTEXES__
#define __QUICK_AND_DIRTY_MUTEXES__

#include <pthread.h>

typedef pthread_mutex_t Mutex;

#define mutexInit(__m) do { pthread_mutex_init(&__m,NULL); pthread_mutex_trylock(&__m); pthread_mutex_unlock(&__m); } while(0)
#define mutexLock(__m) pthread_mutex_lock(&__m)
#define mutexUnlock(__m) pthread_mutex_unlock(&__m)

#endif

