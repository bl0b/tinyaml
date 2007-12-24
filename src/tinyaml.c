#include "vm.h"
#include <tinyap.h>
#include <stdio.h>

#include "_impl.h"

program_t compile_wast(wast_t node, vm_t vm);


void e_init(vm_engine_t e) {}
void e_deinit(vm_engine_t e) {}
void e_kill(vm_engine_t e) {}
void e_run(vm_engine_t e, program_t p, word_t prio) {
	vm_add_thread(e->vm,p,0,prio);
	while(e->vm->threads_count) {
		vm_schedule_cycle(e->vm);
	}
}

struct _vm_engine_t stub_engine = {
	e_init,
	e_deinit,
	e_run,
	e_kill,
	NULL
};



int main(int argc, char** argv) {
	vm_t vm;
	tinyap_t parser;
	program_t p;
	writer_t w;

	tinyap_init();

	parser = tinyap_new();
	vm = vm_new();

	p = vm_compile_file(vm,"tests/test.asm");

	w = file_writer_new("test_asm.bin");
	vm_serialize_program(vm,p,w);
	writer_close(w);

	vm_set_engine(vm, &stub_engine);
	vm_run_program(vm, p, 50);

	return 0;
}

