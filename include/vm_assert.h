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

#ifndef __VM_ASSERT_H__
#define __VM_ASSERT_H__

#include "vm.h"
#include <stdio.h>

/*! \weakgroup vm_assert
 * @{
 */

/*! \brief Is called when an assertion has failed. Kills the concerned thread. */
extern void _vm_assert_fail(const char* assertion, const char*file, unsigned long line, const char* function);

#if defined(NODEBUG)||defined(NDEBUG)
#define assert(_x_)
#else
/*! \brief Evaluate an assertion and catch problems. */
#define assert(_x_)						\
	((_x_)							\
	? ((void)0)						\
	: _vm_assert_fail( "Assertion failed : " #_x_ , __FILE__, __LINE__, __func__ ))
#endif

/*! \brief Trig a failure by hand. */
#define vm_fatal(_str) _vm_assert_fail( _str , __FILE__, __LINE__, __func__ )

/*@}*/

#endif

