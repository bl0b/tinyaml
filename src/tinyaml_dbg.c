#include <curses.h>
#include <string.h>

#include "vm.h"
#include "fastmath.h"
#include "_impl.h"
#include "vm_engine.h"
#include "object.h"
#include "program.h"

void lookup_label_and_ofs(program_t cs, word_t ip, const char** label, word_t* ofs);

static void data_stack_renderer(WINDOW* w,vm_data_t d) {
	_IFC conv;
	/*int i;*/
	/*const unsigned char*s;*/
	switch(d->type) {
	case DataInt:
		wprintw(w,"Int     %li",d->data);
		break;
	case DataFloat:
		conv.i = d->data;
		wprintw(w,"Float   %f",conv.f);
		break;
	case DataString:
		wprintw(w,"String  \"%s\"",(const char*)d->data);
		break;
	case DataObjStr:
		wprintw(w,"ObjStr  \"%s\"",(const char*)d->data);
		break;
	case DataObjSymTab:
		wprintw(w,"SymTab  %p",(void*)d->data);
		break;
	case DataObjMutex:
		wprintw(w,"Mutex  %p",(void*)d->data);
		break;
	case DataObjThread:
		wprintw(w,"Thread  %p",(void*)d->data);
		break;
	case DataObjArray:
		wprintw(w,"Array  %p",(void*)d->data);
		break;
	case DataObjEnv:
		wprintw(w,"Map  %p",(void*)d->data);
		break;
	case DataObjStack:
		wprintw(w,"Stack  %p",(void*)d->data);
		break;
	case DataObjFun:
		wprintw(w,"Function  %p",(void*)d->data);
		break;
	case DataObjVObj:
		wprintw(w,"V-Obj  %p",(void*)d->data);
		break;
	case DataManagedObjectFlag:
		wprintw(w,"Undefined Object ! %p",(void*)d->data);
		break;
	default:
		wprintw(w, "Unknown (%u, 0x%lX)",d->type,d->data);
	};
}

static void closure_stack_renderer(WINDOW* w,dynarray_t* da) {
	vm_data_t tab = (vm_data_t) (*da)->data;
	word_t i;
	wprintw(w,"[%lu]",(*da)->size);
	for(i=0;i<(*da)->size;i+=1) {
		wprintw(w,"\n    %li : ",i);
		data_stack_renderer(w,&(tab[i]));
	}
}

static void call_stack_renderer(WINDOW* w,struct _call_stack_entry_t* cse) {
	word_t ofs;
	const char*label;
	lookup_label_and_ofs(cse->cs,cse->ip,&label,&ofs);
	if(label) {
		wprintw(w,"%p:%lx\n     (%s+%lu)",cse->ip, cse->cs, label, ofs);
	} else {
		wprintw(w,"%p:%lu",cse->cs,cse->ip);
	}
}


static void render_stack(WINDOW* w, generic_stack_t s, word_t sz, const char*prefix, void(*renderer)(WINDOW*,void*)) {
	long counter = 0;
	long stop = -sz;
	wprintw(w,"\n");
	while(counter>stop) {
		wprintw(w,"%s%li : ",prefix,-counter);
		renderer(w,_gpeek(s,counter));
		wprintw(w,"\n");
		counter-=1;
	}
}




/**************************************************************************
 *	    _   _  ____ _   _ ____  ____  _____ ____  
 *	   | \ | |/ ___| | | |  _ \/ ___|| ____/ ___| 
 *	   |  \| | |   | | | | |_) \___ \|  _| \___ \ 
 *	   | |\  | |___| |_| |  _ < ___) | |___ ___) |
 *	   |_| \_|\____|\___/|_| \_\____/|_____|____/ 
 *
 *************************************************************************/

void read_command() {
	getch();
}


int x_sz,y_sz;

WINDOW* call_stack;
WINDOW* data_stack;
WINDOW* data_seg;
WINDOW* locals_stack;
WINDOW* closure_stack;

WINDOW* code;

#define ST_W ((getmaxx(stdscr)>>2))
#define ST_H ((getmaxy(stdscr)>>1))

void print_centered(WINDOW*w, int y, const char* str) {
	mvwprintw(w,y,(getmaxx(w)-strlen(str))>>1, str);
}

void repaint(vm_t vm, thread_t t) {
	word_t ofs;
	vm_data_t tab;
	const char*label;
	const char*disasm;
	word_t code_start, code_end,i,j;
	char title[50];

	clear();

	if(!(vm&&t)) {

		box(code,0,0);
		print_centered(code,0," No running thread ");
		box(call_stack,0,0);

		box(data_stack,0,0);
		box(locals_stack,0,0);
		box(closure_stack,0,0);
		box(data_seg,0,0);

		wnoutrefresh(stdscr);
		doupdate();

		read_command();
		
		return;
	}

	/* fill code window */
	sprintf(title," %s Thread %p CS:%p IP:%lx ",
		t->state==ThreadRunning?"Running":
			t->state==ThreadZombie?"Zombie":
				t->state==ThreadDying?"Dying":
					"TODO",
		t,t->program,t->IP);

	if(t->IP>ST_H) {
		code_start = t->IP-ST_H;
	} else {
		code_start = 0;
	}

	if((long)t->IP>(long)(t->program->code.size-ST_H)) {
		code_end=t->program->code.size;
	} else {
		code_end=t->IP+ST_H;
	}

	wattroff(code,A_STANDOUT);

	j = getmaxx(code)-17;

	for(i=code_start;i<code_end;i+=2) {
		disasm = program_disassemble(vm,t->program,i);
		label = program_lookup_label(t->program,i);
		if(i==t->IP) {
			wattron(code,A_STANDOUT);
		}
		if(label) {
			sprintf(title,"%s :",label);
			wprintw(code," \t%-*.*s\n %8.8lx  =>\t%-*.*s\n",j+8,j+8,title,i,j,j,disasm);
		} else {
			wprintw(code," %8.8lx  =>\t%-*.*s\n",i,j,j,disasm);
		}
		if(i==t->IP) {
			wattroff(code,A_STANDOUT);
		}
		free((char*)disasm);
	}

	box(code,0,0);
	sprintf(title," %s Thread %p CS:%p IP:%lx \n",
		t->state==ThreadRunning?"Running":
			t->state==ThreadZombie?"Zombie":
				t->state==ThreadDying?"Dying":
					"TODO",
		t,t->program,t->IP);
	print_centered(code,0,title);

	/* render call stack */

	sprintf(title," Call stack [%lu] ", gstack_size(&t->call_stack));
	render_stack(call_stack,&t->call_stack, gstack_size(&t->call_stack), "  ", (void(*)(WINDOW*,void*)) call_stack_renderer);
	box(call_stack,0,0);
	print_centered(call_stack,0,title);

	sprintf(title," Data stack [%lu] ", gstack_size(&t->data_stack));
	render_stack(data_stack,&t->data_stack, gstack_size(&t->data_stack), "  ", (void(*)(WINDOW*,void*)) data_stack_renderer);
	box(data_stack,0,0);
	print_centered(data_stack,0,title);

	sprintf(title," Closures stack [%lu] ", gstack_size(&t->closures_stack));
	render_stack(closure_stack,&t->closures_stack, gstack_size(&t->closures_stack), "  ", (void(*)(WINDOW*,void*)) closure_stack_renderer);
	box(closure_stack,0,0);
	print_centered(closure_stack,0,title);

	sprintf(title," Locals stack [%lu] ", gstack_size(&t->closures_stack));
	render_stack(locals_stack,&t->locals_stack, gstack_size(&t->locals_stack), "  ", (void(*)(WINDOW*,void*)) data_stack_renderer);
	box(locals_stack,0,0);
	print_centered(locals_stack,0,title);

	code_end = t->program->data.size>>1;
	sprintf(title," Data segment [%lu] ", code_end);
	tab = (vm_data_t) t->program->data.data;
	for(i=0;i<code_end;i+=1) {
		wprintw(data_seg,"\n  %li : ",i);
		data_stack_renderer(data_seg,&(tab[i]));
	}
	box(data_seg,0,0);
	print_centered(data_seg,0,title);


	wnoutrefresh(stdscr);
	doupdate();

	read_command();

/*	lookup_label_and_ofs(t->program,t->IP,&label,&ofs);
	fprintf(stderr, "Thread :\t%p\n",t);
	if(label) {
		fprintf(stderr,"CS:IP :\t%p:%lXh (%s+%lXh)", t->program, t->IP, label, ofs);
	} else {
		fprintf(stderr,"CS:IP :\t%p:%lXh",t->program,t->IP);
	}
	{
		const char* disasm = program_disassemble(vm,t->program,t->IP);
		printf("\t# %-40.40s\n",disasm);
		free((char*)disasm);
	}*/
/*	fprintf(stderr,"\nCall stack :\t[%lu]\n", gstack_size(&t->call_stack));
	render_stack(&t->call_stack, gstack_size(&t->call_stack), "\t", (void(*)(void*)) call_stack_renderer);
	fprintf(stderr,"\nClosures stack :\t[%lu]\n", gstack_size(&t->data_stack));
	render_stack(&t->closures_stack, gstack_size(&t->closures_stack), "\t", (void(*)(void*)) closure_stack_renderer);
	fprintf(stderr,"\nData stack :\t[%lu]\n", gstack_size(&t->data_stack));
	render_stack(&t->data_stack, t->data_sp_backup, "\t", (void(*)(void*)) data_stack_renderer);
	fprintf(stderr,"\nLocals stack :\t[%lu]\n", gstack_size(&t->locals_stack));
	render_stack(&t->locals_stack, gstack_size(&t->locals_stack), "\t", (void(*)(void*)) data_stack_renderer);
	fprintf(stderr,"\nCatch stack :\t[%lu]\n", gstack_size(&t->catch_stack));
*/
}





void init() {
	initscr(); noecho(); cbreak(); curs_set(0);

	x_sz = getmaxx(stdscr);
	y_sz = getmaxy(stdscr);

	code = subwin(stdscr,y_sz-ST_H, x_sz-ST_W, 0, 0);
	print_centered(code,0," Code segment ");

	call_stack = subwin(stdscr, ST_H, ST_W, 0, x_sz-ST_W);
	print_centered(call_stack,0," Call Stack ");

	data_stack = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, 0);
	print_centered(data_stack,0," Data Stack ");

	locals_stack = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, ST_W);
	print_centered(locals_stack,0," Locals Stack ");

	closure_stack = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, 2*ST_W);
	print_centered(closure_stack,0," Closures Stack ");

	data_seg = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, 3*ST_W);
	print_centered(data_seg,0," Data Segment ");
}

void term() {
	endwin();
}



/**************************************************************************
 *			 __  __    _    ___ _   _ 
 *			|  \/  |  / \  |_ _| \ | |
 *			| |\/| | / _ \  | ||  \| |
 *			| |  | |/ ___ \ | || |\  |
 *			|_|  |_/_/   \_\___|_| \_|
 *                                        
 *************************************************************************/

extern const vm_engine_t debug_engine;

int main(int argc, const char**argv) {
	reader_t r;
	program_t p;
	if(argc==1) {
		return 1;
	}
	vm_t vm = vm_new();
	vm_set_engine(vm,debug_engine);
	init();
	r = file_reader_new(argv[1]);
	p = vm_unserialize_program(vm,r);
	reader_close(r);
	vm_run_program_fg(vm,p,0,50);
	wnoutrefresh(stdscr);
	doupdate();
	getch();
	term();
	vm_del(vm);
	return 0;
}



/**************************************************************************
 *	   __     ____  __     _____ _   _  ____ ___ _   _ _____ 
 *	   \ \   / /  \/  |   | ____| \ | |/ ___|_ _| \ | | ____|
 *	    \ \ / /| |\/| |   |  _| |  \| | |  _ | ||  \| |  _|  
 *	     \ V / | |  | |   | |___| |\  | |_| || || |\  | |___ 
 *	      \_/  |_|  |_|   |_____|_| \_|\____|___|_| \_|_____|
 * 
 *************************************************************************/

void _VM_CALL dbg_run(vm_engine_t e, program_t p, word_t ip, word_t prio) {
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
		vm_push_caller(e->vm,t->program,t->IP,0);
		s_sz = t->call_stack.sp-1;
		t->program=p;
		t->IP=ip;
		while(e->vm->threads_count&&t->call_stack.sp!=s_sz) {
			vm_schedule_cycle(e->vm);
		}
		if(e->vm->threads_count) {
			/*printf("done with sub thread\n");*/
			/* restore old state */
			t->program=program;
			t->IP=IP;
			t->jmp_seg=jmp_seg;
			t->jmp_ofs=jmp_ofs;
		}
	} else {
		vm_add_thread(e->vm,p,ip,prio,0);
		while(e->vm->threads_count) {
			vm_schedule_cycle(e->vm);
		}
	}
}



void _VM_CALL dbg_thread_failed(vm_t vm,thread_t t) {
	t->data_stack.sp = t->data_sp_backup;
	repaint(vm,t);
	getch();
}

void _VM_CALL e_stub(vm_engine_t);

void _VM_CALL dbg_debug(vm_engine_t e) {
	repaint(e->vm,e->vm->current_thread);
}

/* synchronous engine. init lock unlock and deinit are pointless, and kill can't happen. */
const vm_engine_t debug_engine = (struct _vm_engine_t[])
{{
	e_stub,
	e_stub,
	dbg_run,
	e_stub,
	e_stub,
	dbg_run,
	e_stub,
	e_stub,
	e_stub,
	e_stub,
	e_stub,
	dbg_thread_failed,
	dbg_debug,
	NULL
}};



