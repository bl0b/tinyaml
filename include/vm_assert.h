#ifndef __VM_ASSERT_H__
#define __VM_ASSERT_H__

#include "vm.h"
#include <stdio.h>

extern void _vm_assert_fail(const char* assertion, const char*file, unsigned int line, const char* function);

#ifdef NODEBUG
#define assert(_x_)
#else
#define assert(_x_)						\
	((_x_)							\
	? ((void)0)						\
	: _vm_assert_fail( #_x_ , __FILE__, __LINE__, __func__ ))
#endif

#define vm_fatal(_str) _vm_assert_fail( _str , __FILE__, __LINE__, __func__ )


#endif

