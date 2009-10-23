#include "file.h"

void couldnt(const char* what, const char* where);

void _VM_CALL vm_op_isOpen(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	file_t f = (file_t)d->data;
	assert(d->type==DataObjUser&&(f->magic==0x6106F11E||f->magic==0x6106D123));
	vm_push_data(vm, DataObjUser, !!(f->flags&FISOPEN));
}

void _VM_CALL vm_op_atEOF(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	file_t f = (file_t)d->data;
	assert(d->type==DataObjUser&&(f->magic==0x6106F11E||f->magic==0x6106D123));
	if(f->magic==0x6106F11E) {
		vm_push_data(vm, DataInt, !!feof(f->descr.f));
	} else {
		vm_push_data(vm, DataInt, !!f->data);
	}
}

void _VM_CALL vm_op_flush(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	file_t f = (file_t)d->data;
	assert(d->type==DataObjUser&&f->magic==0x6106F11E);
	fflush(f->descr.f);
}

/*void _VM_CALL vm_op_bufferNew(vm_t vm, word_t unused) {*/
	/**/
/*}*/

void _VM_CALL vm_op_seek_Char(vm_t vm, word_t whence) {
	int w;
	vm_data_t d = _vm_pop(vm);
	vm_data_t df = _vm_pop(vm);
	file_t f = (file_t)df->data;
	assert(d->type==DataInt);
	assert(df->type==DataObjUser&&f->magic==0x6106F110);
	if(!(f->flags&FISSEEKABLE)) {
		vm_fatal("File object is not seekable.");
	}
	switch((char)whence) {
	case 'S': w=SEEK_SET; break;
	case 'E': w=SEEK_END; break;
	case 'C': w=SEEK_CUR; break;
	default:;
		vm_fatal("Bad seek origin");
		w=-1; /* make compiler happy. compiler doesn't know that vm_fatal will longjmp. */
	};
	fseek(f->descr.f, (int)d->data, w);
}

void _VM_CALL vm_op_tell(vm_t vm, word_t whence) {
	vm_data_t df = _vm_pop(vm);
	file_t f = (file_t)df->data;
	assert(df->type==DataObjUser&&f->magic==0x6106F110);
	if(!(f->flags&FISSEEKABLE)) {
		vm_fatal("File object is not seekable.");
	}
	vm_push_data(vm, DataInt, ftell(f->descr.f));
}



void _VM_CALL vm_op_opendir(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	const char* path = (const char*)d->data;

	assert(d->type==DataString||d->type==DataObjStr);

	vm_push_data(vm, DataObjUser, (word_t) dir_new(vm, path));
}




void _VM_CALL vm_op_readdir(vm_t vm, word_t unused) {
	vm_data_type_t dt;
	file_t f;
	vm_peek_data(vm, 0, &dt, (word_t*)&f);
	assert(dt==DataObjUser && f->magic==0x6106D123);

	if(f->flags&FCMDDONE) {
		/*vm_printf("DONE READING\n");*/
		pthread_mutex_lock(&f->mutex);
		file_cmd_reset(f);
		vm_pop_data(vm,1);
		if(f->data!=0) {
			vm_push_data(vm, DataObjStr, f->data);
		} else {
			vm_push_data(vm, DataString, (word_t)"");
		}
		pthread_mutex_unlock(&f->mutex);
	} else if(!(f->flags&FCMDMASK)) {
		/*vm_printf("READING...\n");*/
		cmd_unpack(vm, f, 0);
	} else {
		blocker_suspend(vm, f->blocker, vm->current_thread);
	}
}




void _VM_CALL vm_op_fopen_String(vm_t vm, const char* mode) {
	vm_data_t d = _vm_pop(vm);
	/*int fd;*/
	const char* fpath = (const char*)d->data;
	file_t f;
	FILE*F;
	word_t fflags=FISOPEN|FISSEEKABLE;
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
		couldnt("open file", fpath);
	}
	f = file_new(vm, fpath, F,  fflags);
	vm_push_data(vm, DataObjUser, (word_t)f);
}

void _VM_CALL vm_op_fopen(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	vm_op_fopen_String(vm, (const char*)d->data);
}

static inline void addr2str(char* buf, struct sockaddr_in* sa) {
	sprintf(buf, "%u.%u.%u.%u:%u",
		((unsigned char*)&sa->sin_addr.s_addr)[0],
		((unsigned char*)&sa->sin_addr.s_addr)[1],
		((unsigned char*)&sa->sin_addr.s_addr)[2],
		((unsigned char*)&sa->sin_addr.s_addr)[3],
		ntohs(sa->sin_port));
}

static inline file_t sock_init(vm_t vm, int type) {
	struct sockaddr_in sa;
	int sock;
	FILE*F;
	vm_data_t d;
	char buf[22];
	/* create socket */
	if((sock = socket(PF_INET, type, 0))==-1) {
		couldnt("open socket", NULL);
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
		couldnt("connect socket", inet_ntoa(sa.sin_addr));
	}
	addr2str(buf, &sa);
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
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	vm_push_data(vm, DataInt, ntohl(*(word_t*)gethostbyname((const char*)d->data)->h_addr));
}

void _VM_CALL vm_op_ip2string(vm_t vm, word_t unused) {
	struct in_addr ip;
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataInt);
	ip.s_addr = htonl(d->data);
	vm_push_data(vm, DataObjStr, (word_t)vm_string_new(inet_ntoa(ip)));
}

void _VM_CALL vm_op_popen_String(vm_t vm, const char* mode) {
	word_t flags=FISOPEN|FISPROCESS;
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	if(strcmp(mode, "r")&&strcmp(mode, "w")) {
		vm_fatal("Can only popen with 'w' or 'r' modes.");
	}
	flags|= (mode[0]=='r'?FREADABLE:FWRITABLE);
	vm_push_data(vm, DataObjUser, 
		(word_t)file_new(vm, (const char*)d->data,
				popen((const char*)d->data, mode),
				flags));
}

void _VM_CALL vm_op_popen(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	vm_op_popen_String(vm, (const char*)d->data);
}

void _VM_CALL vm_op_close(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	file_t f = (file_t)d->data;
	assert(d->type==DataObjUser&&(f->magic==0x6106F11E||f->magic==0x6106D123));
	if(f->flags&FISOPEN) {
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
		couldnt("stat file", (const char*)d->data);
	}
	vm_push_data(vm, DataInt, (word_t)st.st_size);
}

void _VM_CALL vm_op_ftype(vm_t vm, word_t unused) {
	struct stat st;
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	if(stat((const char*)d->data, &st)) {
		/* stat error */
		couldnt("stat file", (const char*)d->data);
	}
	if(S_ISREG(st.st_mode)) {
		vm_push_data(vm, DataString, (word_t)"file");
	} else if(S_ISDIR(st.st_mode)) {
		vm_push_data(vm, DataString, (word_t)"dir");
	} else if(S_ISLNK(st.st_mode)) {
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
	if(readlink((const char*)d->data, buf, 1024)==-1) {
		couldnt("read symbolic link", (const char*)d->data);
	}
	vm_push_data(vm, DataObjStr, (word_t) vm_string_new(buf));
}

void _VM_CALL vm_op_symlink(vm_t vm, word_t unused) {
	vm_data_t dest = _vm_pop(vm);
	vm_data_t src = _vm_pop(vm);
	assert(dest->type==DataString||dest->type==DataObjStr);
	assert(src->type==DataString||src->type==DataObjStr);
	if(symlink((const char*)src->data, (const char*)dest->data)==-1) {
		couldnt("create symbolic link", (const char*)dest->data);
	}
}

void _VM_CALL vm_op_rename(vm_t vm, word_t unused) {
	vm_data_t dest = _vm_pop(vm);
	vm_data_t src = _vm_pop(vm);
	assert(dest->type==DataString||dest->type==DataObjStr);
	assert(src->type==DataString||src->type==DataObjStr);
	if(rename((const char*)src->data, (const char*)dest->data)==-1) {
		couldnt("rename file", (const char*)dest->data);
	}
}

void _VM_CALL vm_op_unlink(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	if(unlink((const char*)d->data)==-1) {
		couldnt("unlink file", (const char*)d->data);
	}
}

void _VM_CALL vm_op_fsource(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	file_t f = (file_t)d->data;
	assert(d->type==DataObjUser&&(f->magic==0x6106F11E||f->magic==0x6106D123));
	vm_push_data(vm, DataObjStr, (word_t)vm_string_new(f->source));
}

void _VM_CALL vm_op_mkdir(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	if(mkdir((const char*)d->data, 0777)==-1) {	/* trust umask */
		couldnt("create directory", (const char*)d->data);
	}
}

void _VM_CALL vm_op_getcwd(vm_t vm, word_t unused) {
	char buf[1024];
	if(getcwd(buf, 1024)==NULL) {
		couldnt("get current working directory", NULL);
	}
	vm_push_data(vm, DataObjStr, (word_t) vm_string_new(buf));
}

void _VM_CALL vm_op_chdir(vm_t vm, word_t unused) {
	vm_data_t d = _vm_pop(vm);
	assert(d->type==DataString||d->type==DataObjStr);
	if(chdir((const char*)d->data)==-1) {
		couldnt("chdir to", (const char*)d->data);
	}
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


void _VM_CALL vm_op___tcpserver(vm_t vm, word_t unused) {
	struct sockaddr_in sa;
	int sock;
	int backlog;
	vm_data_t d;
	char buf[22];
	/* create socket */
	if((sock = socket(PF_INET, SOCK_STREAM, 0))==-1) {
		couldnt("open socket", NULL);
	}
	/* set backlog */
	d = _vm_pop(vm);
	assert(d->type==DataInt);
	backlog = (int)d->data;
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
	bind(sock, (struct sockaddr*)&sa, sizeof(struct sockaddr_in));
	listen(sock, backlog);
	addr2str(buf, &sa);
	vm_push_data(vm, DataObjUser, (word_t)file_new(vm, buf, (FILE*)sock, FISOPEN|FREADABLE|FISTCPSERVER));
}

void _VM_CALL vm_op___accept(vm_t vm, word_t unused) {
	vm_data_type_t dt;
	file_t server;
	char buf[22];
	vm_peek_data(vm, 0, &dt, (word_t*)&server);
	assert(dt==DataObjUser);
	assert(server->magic==0x6106F11E);
	/*vm_printerrf("server type flags %X (vs. %X)\n", _file_type(server), FISTCPSERVER);*/
	assert(file_is_tcpserver(server));
	if(server->flags&FCMDDONE) {
		file_cmd_reset(server);
		if(server->data!=-1) {
			vm_printerrf("[TCPSRV] New client ! fd=%li\n", server->data);
			vm_push_data(vm, DataInt, (word_t)server->extra.client.addr);
			vm_push_data(vm, DataInt, (word_t)server->extra.client.port);
			sprintf(buf, "%u.%u.%u.%u:%u",
				((unsigned char*)&server->extra.client.addr)[0],
				((unsigned char*)&server->extra.client.addr)[1],
				((unsigned char*)&server->extra.client.addr)[2],
				((unsigned char*)&server->extra.client.addr)[3],
				server->extra.client.port);
			vm_push_data(vm, DataObjUser, (word_t)file_new(vm, buf, fdopen(server->data, "r+"), FISSOCKET|FISOPEN|FREADABLE|FWRITABLE));
		} else {
			vm_push_data(vm, DataInt, (word_t)-1);
			vm_push_data(vm, DataInt, (word_t)-1);
			vm_push_data(vm, DataInt, (word_t)-1);
		}
	} else if(server->flags&FCMDREAD) {
		blocker_suspend(vm, server->blocker, vm->current_thread);
	} else {
		cmd_unpack(vm, server, (char)0);
	}
}


