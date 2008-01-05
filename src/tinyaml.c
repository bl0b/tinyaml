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
#include <string.h>

#include "_impl.h"

#include "fastmath.h"


#define TINYAML_ABOUT	"This is not yet another meta-language.\n" \
			"(c) 2007-2008 Damien 'bl0b' Leroux\n\n"

program_t compile_wast(wast_t node, vm_t vm);


void e_init(vm_engine_t e) {}
void e_deinit(vm_engine_t e) {}
void e_kill(vm_engine_t e) {}
void e_run(vm_engine_t e, program_t p, word_t ip, word_t prio) {
	if(e->vm->current_thread) {
		thread_t t = e->vm->current_thread;
		/* save some thread state */
		program_t jmp_seg = t->jmp_seg;
		word_t jmp_ofs = t->jmp_ofs;
		word_t IP = t->IP;
		program_t program = t->program;
		/* hack new thread state */
		word_t s_sz;
		t->jmp_ofs=0;
		t->jmp_seg=p;
		vm_push_caller(e->vm,t->program,t->IP);
		s_sz = t->call_stack.sp-1;
		t->program=p;
		t->IP=ip;
		while(t->call_stack.sp!=s_sz) {
			vm_schedule_cycle(e->vm);
		}
		/*printf("done with sub thread\n");*/
		/* restore old state */
		t->program=program;
		t->IP=IP;
		t->jmp_seg=jmp_seg;
		t->jmp_ofs=jmp_ofs;
	} else {
		vm_add_thread(e->vm,p,ip,prio);
		while(e->vm->threads_count) {
			vm_schedule_cycle(e->vm);
		}
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




#define cmp_param(_n,_arg_str_long,_arg_str_short) (i<(argc-_n) && (	\
		(_arg_str_long && !strcmp(_arg_str_long,argv[i]))	\
					||				\
		(_arg_str_short && !strcmp(_arg_str_short,argv[i]))	\
	))

static int tinyaml_quiet=0;

int do_args(vm_t vm, int argc,char*argv[]) {
	int i;
	program_t p=NULL;
	writer_t w=NULL;
	reader_t r=NULL;

	for(i=1;i<argc;i+=1) {
		if(cmp_param(1,"--compile","-c")) {
			i+=1;
			p = vm_compile_file(vm,argv[i]);
		} else if(cmp_param(1,"--save","-s")) {
			i+=1;
			w = file_writer_new(argv[i]);
			vm_serialize_program(vm,p,w);
			writer_close(w);
		} else if(cmp_param(1,"--load","-l")) {
			i+=1;
			r = file_reader_new(argv[i]);
			p=vm_unserialize_program(vm,r);
			reader_close(r);
		} else if(cmp_param(0,"--quiet","-q")) {
			tinyaml_quiet = 1;
		} else if(cmp_param(0,"--properties","-p")) {
			program_dump_stats(p);
		} else if(cmp_param(0,"--run-foreground","-f")) {
			vm_run_program_fg(vm,p,0,50);
		} else if(cmp_param(0,"--run-background","-b")) {
			vm_run_program_bg(vm,p,0,50);
		} else if(cmp_param(0,"--disasm","-d")) {
			/*fputs("Disassembling not yet implemented.\n",stdout);*/
			for(i=0;i<p->code.size;i+=2) {
				const char* label = program_lookup_label(p,i);
				const char* disasm = program_disassemble(vm,p,i);
				printf("%8.8lX %-20.20s%s %-50.50s\n",(long)i,label?label:"",label&&*label?":":" ",disasm);
				free((char*)disasm);
	
			}
		} else if(cmp_param(0,"--version","-v")) {
			printf(TINYAML_ABOUT);
			printf("version " TINYAML_VERSION "\n" );
			exit(0);
		} else if(cmp_param(0,"--help","-h")) {
			printf(TINYAML_ABOUT);
			printf("Usage : %s [--compile, -c [filename]] [--save, -s [filename]] [--load, -l [filename]] [--run-foreground, -f] [--run-background, -b] [--version, -v] [--help,-h]\n",argv[0]);
			printf(	"Commands are executed on the fly.\n"
				"\n\t--compile,-c [filename]\tcompile this file\n"
				  "\t--save,-s [filename]\tsave the newest program into this file\n"
				  "\t--load,-l [filename]\tload a serialized program from this file\n"
				  "\t--run-foreground,-f \trun the newest program in foreground\n"
				  "\t--run-background,-b \trun the newest program in background\n"
				  "\t--version,-v \t\tdisplay program version\n"
				"\n\t--help,-h\t\tdisplay this text\n\n");
			exit(0);
		}
	}

	return 0;
}










int main(int argc, char** argv) {
	vm_t vm;
	vm = vm_new();
	vm_set_engine(vm, &stub_engine);
	do_args(vm,argc,argv);
	if(!tinyaml_quiet) {
		printf("\nVM runned for %lu cycles.\n",vm->cycles);
	}
	vm_del(vm);
	return 0;
}

