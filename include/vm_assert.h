#ifndef __VM_ASSERT_H__
#define __VM_ASSERT_H__

#include "vm.h"
#include <stdio.h>

/*! \addtogroup misc
 * @{
 * \addtogroup assert Assertions
 * @{
 */

/*! \brief Is called when an assertion has failed. Kills the concerned thread. */
extern void _vm_assert_fail(const char* assertion, const char*file, unsigned int line, const char* function);

#ifdef NODEBUG
#define assert(_x_)
#else
/*! \brief Evaluate an assertion and catch problems. */
#define assert(_x_)						\
	((_x_)							\
	? ((void)0)						\
	: _vm_assert_fail( #_x_ , __FILE__, __LINE__, __func__ ))
#endif

/*! \brief Trig a failure by hand. */
#define vm_fatal(_str) _vm_assert_fail( _str , __FILE__, __LINE__, __func__ )

/*@}*/
/*@}*/

#endif

