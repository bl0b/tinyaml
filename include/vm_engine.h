
#ifndef _BML_VM_ENGINE_H_
#define _BML_VM_ENGINE_H_

#include "vm_types.h"

typedef void(* vm_engine_func_t )(vm_engine_t);


vm_engine_t vm_engine_archetype_new(vm_engine_func_t _init, vm_engine_func_t _run, vm_engine_func_t _pause, vm_engine_func_t _stop, vm_engine_func_t _free);
vm_engine_t vm_engine_new(vm_engine_t archetype, vm_t);
void vm_engine_del(vm_engine_t);

#endif

