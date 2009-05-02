#include "file.h"

void _VM_CALL vm_op_isOpen(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	file_t f = (file_t)d->data;
	vm_push_data(vm, DataObjUser, !!(f->flags&FISOPEN));
}

void _VM_CALL vm_op_bufferNew(vm_t vm, word_t unused) {
	
}

void _VM_CALL vm_op_fopen_String(vm_t vm, const char* mode) {
	vm_data_t d = _vm_pop(vm);
	int fd;
	const char* fpath = (const char*)d->data;
	file_t f;
	FILE*F;
	int flags=0;
	word_t fflags=FISOPEN;
	assert(d->type==DataString||d->type==DataObjStr);
	assert(mode[0]!=0);
	switch(mode[0]) {
	case 'a':
		/*flags=O_CREAT|O_APPEND;*/
		if(mode[1]=='+') {
			/*flags|=O_RDWR;*/
			fflags|=FREADABLE|FWRITABLE;
		} else {
			/*flags|=O_WRONLY;*/
			fflags|=FWRITABLE;
		}
		break;
	case 'w':
		/*flags|=O_CREAT|O_TRUNC;*/
		if(mode[1]=='+') {
			/*flags|=O_RDWR;*/
			fflags|=FREADABLE|FWRITABLE;
		} else {
			/*flags|=O_WRONLY;*/
			fflags|=FWRITABLE;
		}
		break;
	case 'r':
		if(mode[1]=='+') {
			/*flags=O_RDWR;*/
			fflags|=FREADABLE|FWRITABLE;
		} else {
			/*flags=O_RDONLY;*/
			fflags|=FREADABLE;
		}
	};

	/*fd = open(fpath, flags, 0660);*/
	/*if(fd==-1) {*/
		/*char buf[1024];*/
		/*sprintf(buf, "Couldn't open file '%s'.", fpath);*/
		/*vm_fatal(buf);*/
	/*}*/
	F = fopen(fpath, mode);
	if(F==NULL) {
		char buf[1024];
		sprintf(buf, "Couldn't open file '%s'.", fpath);
		vm_fatal(buf);
	}
	f = file_new(vm, fpath, F,  fflags);
	vm_push_data(vm, DataObjUser, (word_t)f);
}

void _VM_CALL vm_op_fopen(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	vm_op_fopen_String(vm, (const char*)d->data);
}

static inline file_t sock_init(vm_t vm, int type) {
	struct sockaddr_in sa;
	unsigned long int ip;
	int sock;
	FILE*F;
	unsigned short port;
	vm_data_t d;
	char buf[22];
	/* create socket */
	if((sock = socket(PF_INET, type, 0))==-1) {
		vm_fatal("Couldn't open socket.");
	}
	/* set port */
	d = _vm_pop(vm);
	assert(d->type==DataInt);
	sa.sin_port = htons((unsigned short)d->data);
	/* set IP */
	d = _vm_pop(vm);
	assert(d->type==DataInt);
	sa.sin_addr.s_addr = htonl(d->data);
	/* set family */
	sa.sin_family = AF_INET;
	/* connect */
	if(connect(sock, (struct sockaddr*)&sa, sizeof(struct sockaddr_in))==-1) {
		vm_fatal("Couldn't connect socket.");
	}
	sprintf(buf, "%u.%u.%u.%u:%u",
		((unsigned char*)&sa.sin_addr.s_addr)[0],
		((unsigned char*)&sa.sin_addr.s_addr)[1],
		((unsigned char*)&sa.sin_addr.s_addr)[2],
		((unsigned char*)&sa.sin_addr.s_addr)[3],
		ntohs(sa.sin_port));
	F = fdopen(sock, "r+");
	return file_new(vm, buf, F, FISSOCKET|FISOPEN|FWRITABLE|FREADABLE);
}

void _VM_CALL vm_op_udpopen(vm_t vm, word_t unused) {
	file_t f = sock_init(vm, SOCK_DGRAM);
	vm_push_data(vm, DataObjUser, (word_t)f);
}

void _VM_CALL vm_op_tcpopen(vm_t vm, word_t unused) {
	file_t f = sock_init(vm, SOCK_STREAM);
	vm_push_data(vm, DataObjUser, (word_t)f);
}

void _VM_CALL vm_op_string2ip(vm_t vm, word_t unused) {
	struct hostent* he;
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	he = gethostbyname((const char*)d->data);
	vm_push_data(vm, DataInt, ntohl(*(word_t*)he->h_addr));
}

void _VM_CALL vm_op_ip2string(vm_t vm, word_t unused) {
	struct in_addr ip;
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	ip.s_addr = htonl(d->data);
	vm_push_data(vm, DataObjStr, (word_t)vm_string_new(inet_ntoa(ip)));
}

void _VM_CALL vm_op_popen(vm_t vm, word_t unused) {
	vm_printerrf("[VM:ERR] popen is not implemented yet.\n");
	vm_push_data(vm, 0, 0);
}

void _VM_CALL vm_op_close(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	file_t f = (file_t)d->data;
	assert(d->type==DataObjUser&&f->magic==0x6106F11E);
	if((f->flags&FISOPEN)!=0 && (f->flags&FISSYSTEM)==0) {
		/*vm_printf("CLOSING I/O STREAM %s.\n", f->source);*/
		file_update_state(f, f->flags&~(FISOPEN|FCMDMASK));
		pthread_mutex_lock(&f->mutex);
		while(f->flags&FISRUNNING) {
			pthread_cond_wait(&f->cond, &f->mutex);
		}
		pthread_mutex_unlock(&f->mutex);
	}
}

static inline void __funpack(vm_t vm, unsigned char fmt, int stack_ofs) {
	vm_data_type_t dt;
	file_t f;
	word_t x;
	vm_peek_data(vm, stack_ofs, &dt, (word_t*)&f);
	assert(dt==DataObjUser && f->magic==0x6106F11E);

	if(f->flags&FCMDDONE) {
		/*vm_printf("DONE READING\n");*/
		pthread_mutex_lock(&f->mutex);
		file_cmd_reset(f);
		vm_pop_data(vm,1-stack_ofs);
		switch(fmt) {
		case 'C':
			dt=DataChar;
			break;
		case 'S':
		case 's':
			dt=DataObjStr;
			break;
		case 'I':
		case 'X':
		case 'i':
		case 'b':
			dt=DataInt;
			break;
		case 'F':
		case 'f':
			dt=DataFloat;
			break;
		default:
			vm_fatal("Unhandled format character");
		};
		vm_push_data(vm, dt, f->data);
		pthread_mutex_unlock(&f->mutex);
	} else if(!(f->flags&FCMDMASK)) {
		/*vm_printf("READING...\n");*/
		cmd_unpack(vm, f, fmt);
	} else {
		blocker_suspend(vm, f->blocker, vm->current_thread);
	}
}

static inline void __fpack(vm_t vm, unsigned char fmt, int stack_ofs) {
	vm_data_type_t dt;
	file_t f;
	word_t x;
	vm_peek_data(vm, stack_ofs, &dt, (word_t*)&f);
	assert(dt==DataObjUser && f->magic==0x6106F11E);

	if(f->flags&FCMDDONE) {
		/*file_disable_thread(f);*/
		pthread_mutex_lock(&f->mutex);
		file_cmd_reset(f);
		vm_pop_data(vm,2-stack_ofs);
		/*vm_printf("DONE PRINTING\n");*/
		pthread_mutex_unlock(&f->mutex);
	} else if(!(f->flags&FCMDMASK)) {
		/*vm_printf("FPRINT\n");*/
		vm_peek_data(vm, stack_ofs-1,&dt,&x);
		switch(fmt) {
		case 'S':
		case 's':
			assert(dt==DataString||dt==DataObjStr);
			break;
		case 'C':
		case 'b':
			assert(dt==DataInt||dt==DataChar);
			break;
		case 'I':
		case 'i':
		case 'X':
			assert(dt==DataInt);
			break;
		case 'F':
		case 'f':
			assert(dt==DataFloat);
		};
		cmd_pack(vm, f, fmt, x);
	} else {
		blocker_suspend(vm, f->blocker, vm->current_thread);
	}
}

void _VM_CALL vm_op__fpack_Char(vm_t vm, word_t fmt) {
	__fpack(vm, (unsigned char) fmt, 0);
}

void _VM_CALL vm_op__fpack(vm_t vm, word_t unused) {
	vm_data_type_t dt;
	word_t fmt;
	vm_peek_data(vm, 0, &dt, &fmt);
	assert(dt==DataChar);
	__fpack(vm, (unsigned char) fmt, -1);
}

void _VM_CALL vm_op__funpack_Char(vm_t vm, word_t fmt) {
	__funpack(vm, (unsigned char)fmt, 0);
}

void _VM_CALL vm_op__funpack(vm_t vm, word_t unused) {
	vm_data_type_t dt;
	word_t fmt;
	vm_peek_data(vm, 0, &dt, &fmt);
	assert(dt==DataChar);
	__funpack(vm, (unsigned char) fmt, -1);
}

void _VM_CALL vm_op_fprint(vm_t vm, word_t unused) {
	vm_data_type_t dt;
	file_t f;
	word_t x;
	vm_peek_data(vm,0, &dt, (word_t*)&f);
	assert(dt==DataObjUser && f->magic==0x6106F11E);

	if(f->flags&FCMDDONE) {
		/*file_disable_thread(f);*/
		pthread_mutex_lock(&f->mutex);
		file_cmd_reset(f);
		vm_pop_data(vm,2);
		/*vm_printf("DONE PRINTING\n");*/
		pthread_mutex_unlock(&f->mutex);
	} else if(!(f->flags&FCMDMASK)) {
		/*vm_printf("FPRINT\n");*/
		vm_peek_data(vm, -1,&dt,&x);
		switch(dt) {
		case DataChar:
			cmd_pack(vm, f, 'C', x);
			break;
		case DataInt:
			cmd_pack(vm, f, 'I', x);
			break;
		case DataFloat:
			cmd_pack(vm, f, 'F', x);
			break;
		case DataString:
		case DataObjStr:
			cmd_pack(vm, f, 'S', x);
			break;
		default:
			vm_fatal("Can't print managed object to file.");
		};
	} else {
		blocker_suspend(vm, f->blocker, vm->current_thread);
	}
}

void _VM_CALL vm_op_fsize(vm_t vm, word_t unused) {
	struct stat st;
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	if(stat((const char*)d->data, &st)) {
		/* stat error */
		vm_fatal("Couldn't stat file.");
	}
	vm_push_data(vm, DataInt, (word_t)st.st_size);
}

void _VM_CALL vm_op_ftype(vm_t vm, word_t unused) {
	struct stat st;
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	if(stat((const char*)d->data, &st)) {
		/* stat error */
		vm_fatal("Couldn't stat file.");
	}
	if(S_ISREG(st.st_mode)) {
		vm_push_data(vm, DataString, (word_t)"file");
	} else if(S_ISDIR(st.st_mode)) {
		vm_push_data(vm, DataString, (word_t)"dir");
	} else if(S_ISLINK(st.st_mode)) {
		vm_push_data(vm, DataString, (word_t)"link");
	} else if(S_ISFIFO(st.st_mode)) {
		vm_push_data(vm, DataString, (word_t)"fifo");
	} else if(S_ISSOCK(st.st_mode)) {
		vm_push_data(vm, DataString, (word_t)"socket");
	} else {
		vm_push_data(vm, DataString, (word_t)"unhandled");
	}
}

void _VM_CALL vm_op_readlink(vm_t vm, word_t unused) {
	char buf[1024];
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	readlink((const char*)d->data, buf, 1024);
	vm_push_data(vm, DataObjStr, (word_t) vm_string_new(buf));
}

void _VM_CALL vm_op_symlink(vm_t vm, word_t unused) {
	vm_data_t dest = _vm_pop(vm);
	vm_data_t src = _vm_pop(vm);
	assert(dest->type==DataString||dest->type==DataObjStr);
	assert(src->type==DataString||src->type==DataObjStr);
	symlink((const char*)src->data, (const char*)dest->data);
}

void _VM_CALL vm_op_rename(vm_t vm, word_t unused) {
	vm_data_t dest = _vm_pop(vm);
	vm_data_t src = _vm_pop(vm);
	assert(dest->type==DataString||dest->type==DataObjStr);
	assert(src->type==DataString||src->type==DataObjStr);
	rename((const char*)src->data, (const char*)dest->data);
}

void _VM_CALL vm_op_unlink(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	unlink((const char*)d->data);
}

void _VM_CALL vm_op_fsource(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	file_t f = (file_t)d->data;
	assert(d->type==DataObjUser&&f->magic==0x6106F11E);
	vm_push_data(vm, DataObjStr, (word_t)vm_string_new(f->source));
}

void _VM_CALL vm_op_mkdir(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	mkdir((const char*)d->data, 0777);	/* trust umask */
}

void _VM_CALL vm_op_getcwd(vm_t vm, word_t unused) {
	char buf[1024];
	getcwd(buf, 1024);
	vm_push_data(vm, DataObjStr, (word_t) vm_string_new(buf));
}

void _VM_CALL vm_op_chdir(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	chdir((const char*)d->data);
}

void _VM_CALL vm_op_stdout(vm_t vm, word_t unused) {
	vm_push_data(vm, DataObjUser, (word_t)f_out);
}

void _VM_CALL vm_op_stderr(vm_t vm, word_t unused) {
	/*vm_printf("get stderr stream @%p (magic %X)\n", f_err, f_err->magic);*/
	vm_push_data(vm, DataObjUser, (word_t)f_err);
}

void _VM_CALL vm_op_stdin(vm_t vm, word_t unused) {
	vm_push_data(vm, DataObjUser, (word_t)f_in);
}


void _VM_CALL vm_op___IO__init(vm_t vm, word_t unused) {
	/*vm_printf("Initializing system streams...\n");*/
	f_in = file_new(vm, "<stdin>", stdin, FISFILE|FISOPEN|FISSYSTEM|FREADABLE);
	vm_obj_ref_ptr(vm, f_in);
	f_out = file_new(vm, "<stdout>", stdout, FISFILE|FISOPEN|FISSYSTEM|FWRITABLE);
	vm_obj_ref_ptr(vm, f_out);
	f_err = file_new(vm, "<stderr>", stderr, FISFILE|FISOPEN|FISSYSTEM|FWRITABLE);
	vm_obj_ref_ptr(vm, f_err);
	vm_op_stdin(vm, 0);
	vm_op_stdout(vm, 0);
	vm_op_stderr(vm, 0);
}



