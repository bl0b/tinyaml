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
#include "vm_engine.h"

#include "fastmath.h"

#include <sys/stat.h>

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>


#define TINYAML_ABOUT	"This is not yet another meta-language.\n" \
			"(c) 2007-2008 Damien 'bl0b' Leroux\n\n"

program_t compile_wast(wast_t node, vm_t vm);





#define cmp_param(_n,_arg_str_long,_arg_str_short) (i<(argc-_n) && (	\
		(_arg_str_long && !strcmp(_arg_str_long,argv[i]))	\
					||				\
		(_arg_str_short && !strcmp(_arg_str_short,argv[i]))	\
	))

static int tinyaml_quiet=0;

extern volatile int _vm_trace;

char* interpreter_prefix="";
char* interpreter_suffix="";
int interpreter_multiline=0;
int interpreter_running=0;

int eval_buffer(vm_t vm, program_t p, const char* buf) {
	if(!strncmp(".mode ", buf, 6)) {
		const char* mode = buf+6;
		if(!strcmp(mode, "raw")) {
			interpreter_prefix="";
			interpreter_suffix="";
		} else if(!strcmp(mode, "asm")) {
			interpreter_prefix="asm ";
			interpreter_suffix=" end";
		} else if(!strcmp(mode, "script")) {
			interpreter_prefix="script ";
			interpreter_suffix=" end";
		} else {
			vm_printf("Please use 'asm' or 'raw' or 'script' after .mode\n");
		}
	} else if(!strncmp(".help", buf, 5)) {
		vm_printf("Special commands :\n"
			"	.help		display this text\n"
			"	.mode asm	wrap input inside an asm...end bloc\n"
			"	.mode script	wrap input insode a script...end bloc\n"
			"	.mode raw	don't wrap input\n"
			"	.multi on	enable multi-line input (empty line ends input)\n"
			"	.multi off	disable multi-line input\n"
			"	.quit		quit the interpreter\n"
		);
	} else if(!strcmp(".quit", buf)) {
		interpreter_running=0;
	} else if(!strncmp(".multi ", buf, 7)) {
		if(!strcmp("on", buf+7)) {
			interpreter_multiline=1;
		} else if(!strcmp("off", buf+7)) {
			interpreter_multiline=0;
		} else {
			vm_printf("Please use 'on' or 'off' after .multi\n");
		}
	} else {
		int prelen = strlen(interpreter_prefix);
		int suflen = strlen(interpreter_suffix);
		word_t IP;
		char* interpreter_buffer = (char*) malloc(strlen(buf)+prelen+suflen+3);
		sprintf(interpreter_buffer, "%s%s%s", interpreter_prefix, buf, interpreter_suffix);
		vm->current_edit_prg = p;
		p = vm_compile_append_buffer(vm, interpreter_buffer, &IP, 1);
		/*printf("exec @%li\n", IP);*/
		if(!vm->compile_error) {
			vm_run_program_fg(vm, p, IP<<1, 50);
		}
		return vm->compile_error;
	}
	return 0;
}

void default_error_handler_no_exit(vm_t vm, const char* input, int is_buffer);

int do_args(vm_t vm, int argc,char*argv[]) {
	int i,k;
	program_t p=NULL;
	writer_t w=NULL;
	reader_t r=NULL;

	for(i=1;i<argc;i+=1) {
		if(cmp_param(0,"--trace","-t")) {
			_vm_trace=1;
		} else if(cmp_param(0, "--interactive","-i")) {
			char* line, * pending = NULL;
			program_t p = vm->current_edit_prg;
			vm_printf("Enter .help for help.\n");
			if(!p) {
				p = vm_compile_buffer(vm, "require \"script.wc\" asm end");
			}
			vm->compile_reent+=1;
			vm_error_handler prev_hndl = vm_get_error_handler(vm);
			vm_set_error_handler(vm, default_error_handler_no_exit);
			using_history();
			interpreter_running=1;
			while(interpreter_running && (line=readline(pending?"...  ":"\\_o< "))) {
				if(interpreter_multiline) {
					if(pending) {
						if(strlen(line)>0) {
							pending = (char*)realloc(pending, strlen(pending)+strlen(line)+2);
							strcat(pending, "\n");
							strcat(pending, line);
						}
					} else {
						pending=strdup(line);
					}
					if(strlen(line)==0) {
						eval_buffer(vm, p, pending);
						add_history(pending);
						free(pending);
						pending=NULL;
					}
				} else {
					eval_buffer(vm, p, line);
					add_history(line);
				}
				free(line);
			}
			vm->compile_reent-=1;
			vm_set_error_handler(vm, prev_hndl);
		} else if(cmp_param(0,"--no-trace","-nt")) {
			_vm_trace=0;
		} else if(cmp_param(1,"--timeslice","-ts")) {
			i+=1;
			vm_set_timeslice(vm, atoi(argv[i]));
		} else if(cmp_param(1,"--compile","-c")) {
			i+=1;
			p = vm_compile_file(vm,argv[i]);
			program_set_source(p, argv[i]);
		} else if(cmp_param(1,"--save","-s")) {
			i+=1;
			w = file_writer_new(argv[i]);
			vm_serialize_program(vm,p,w);
			writer_close(w);
			chmod(argv[i], 0755);
		} else if(cmp_param(1,"--load","-l")) {
			i+=1;
			r = file_reader_new(argv[i]);
			p=vm_unserialize_program(vm,r);
			reader_close(r);
			program_set_source(p, argv[i]);
		} else if(cmp_param(1,"--execute","-x")) {
			i+=1;
			r = file_reader_new(argv[i]);
			p=vm_unserialize_program(vm,r);
			reader_close(r);
			vm_run_program_fg(vm,p,0,50);
		} else if(cmp_param(1,"--execute-bg","-z")) {
			i+=1;
			r = file_reader_new(argv[i]);
			p=vm_unserialize_program(vm,r);
			reader_close(r);
			if(vm->engine!=thread_engine) {
				vm_set_engine(vm,thread_engine);
			}
			vm_run_program_bg(vm,p,0,50);
		} else if(cmp_param(0,"--quiet","-q")) {
			tinyaml_quiet = 1;
		} else if(cmp_param(0,"--properties","-p")) {
			program_dump_stats(p);
		} else if(cmp_param(0,"--run-foreground","-f")) {
			vm_run_program_fg(vm,p,0,50);
		} else if(cmp_param(0,"--run-background","-b")) {
			if(vm->engine!=thread_engine) {
				vm_set_engine(vm,thread_engine);
			}
			vm_run_program_bg(vm,p,0,50);
		} else if(cmp_param(0,"--disasm","-d")) {
			/*fputs("Disassembling not yet implemented.\n",stdout);*/
			for(k=0;k<p->code.size;k+=2) {
				const char* label = program_lookup_label(p,k);
				const char* disasm = program_disassemble(vm,p,k);
				vm_printf("%8.8lX %-32.32s%s %-60.60s\n",(long)k,label?label:"",label&&*label?":":" ",disasm);
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
				  "\t--trace,-t \t\tstart tracing execution cycles\n"
				  "\t--no-trace,-nt \t\tstop tracing execution cycles\n"
				  "\t--version,-v \t\tdisplay program version\n"
				"\n\t--help,-h\t\tdisplay this text\n\n");
			exit(0);
		} else {	/* default to execute/compile filename */
			static char buf[1024];
			sprintf(buf, "require \"%s\"", argv[i]);
			p=vm_compile_buffer(vm, buf);
			/*vm_run_program_fg(vm, p, 0, 50);*/
			/*program_free(vm, p);*/
			p=NULL;
			/* convenience hack : default to quiet when invoked to execute files */
			tinyaml_quiet = 1;
		}
	}

	return 0;
}










int main(int argc, char** argv) {
	vm_t vm;
	vm = vm_new();
	/*vm_set_engine(vm, stub_engine);*/
	/*vm_set_engine(vm, thread_engine);*/
	do_args(vm,argc,argv);
	if(!tinyaml_quiet) {
		vm_printf("\nVM runned for %lu cycles.\n",vm->cycles);
	}
	vm_del(vm);
	return 0;
}

