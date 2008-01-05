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


#ifndef _BML_VM_ENGINE_H_
#define _BML_VM_ENGINE_H_

#include "vm_types.h"

typedef void(* vm_engine_func_t )(vm_engine_t);


vm_engine_t vm_engine_archetype_new(vm_engine_func_t _init, vm_engine_func_t _run, vm_engine_func_t _pause, vm_engine_func_t _stop, vm_engine_func_t _free);
vm_engine_t vm_engine_new(vm_engine_t archetype, vm_t);
void vm_engine_del(vm_engine_t);


extern const vm_engine_t
	stub_engine,
	thread_engine;

#endif

