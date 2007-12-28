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

#include "vm.h"
#include "program.h"
#include <tinyap.h>
#include <stdio.h>

#include "_impl.h"

#include "fastmath.h"

program_t compile_wast(wast_t node, vm_t vm);


void e_init(vm_engine_t e) {}
void e_deinit(vm_engine_t e) {}
void e_kill(vm_engine_t e) {}
void e_run(vm_engine_t e, program_t p, word_t ip, word_t prio) {
	vm_add_thread(e->vm,p,ip,prio);
	while(e->vm->threads_count) {
		vm_schedule_cycle(e->vm);
	}
}

struct _vm_engine_t stub_engine = {
	e_init,
	e_deinit,
	e_run,
	e_run,
	e_kill,
	NULL
};



int main(int argc, char** argv) {
	vm_t vm;
	program_t p;
	writer_t w;
	/*wast_t wa;*/

	tinyap_init();

	vm = vm_new();

	vm_set_engine(vm, &stub_engine);

	p = vm_compile_file(vm,"tests/test_ml.asm");
	if(p) {
		w = file_writer_new("test_ml.bin");
		vm_serialize_program(vm,p,w);
		writer_close(w);
/*		printf("Parser output dump : \n");
		wa = tinyap_make_wast(tinyap_list_get_element(tinyap_get_output(vm->parser),0));
		tinyap_walk(wa,"prettyprint",NULL);
		tinyap_free_wast(wa);
		printf("====== END OF DUMP ======\n");
*/
		vm_run_program_fg(vm, p, 0, 50);

		/*p = vm_compile_buffer(vm,"Hello, world.");*/
		p = vm_compile_file(vm,"tests/test2.asm");

		if(p) {
			w = file_writer_new("test_asm.bin");
			printf("Parser output dump : \n");
/*			wa = tinyap_make_wast(tinyap_list_get_element(tinyap_get_output(vm->parser),0));
			tinyap_walk(wa,"prettyprint",NULL);
			tinyap_free_wast(wa);
			printf("====== END OF DUMP ======\n");
*/
			vm_serialize_program(vm,p,w);
			writer_close(w);

			vm_run_program_fg(vm, p, 0, 50);
		}
	}

	printf("VM Runned for %lu cycles.\n",vm->cycles);

	vm_del(vm);

	return 0;
}

