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

#include "file.h"


volatile file_t f_out=NULL, f_in=NULL, f_err=NULL;


long _VM_CALL _fill_buf0(FILE*f, char*buf) {
	long i;
	for(i=0;i<BUFSZ&&fread(buf, 1, 1, f)==1&&*buf;i+=1) buf+=1;
	return i;
}

long _VM_CALL _fill_bufNL(FILE*f, char*buf) {
	long i;
	for(i=0;i<BUFSZ&&fread(buf, 1, 1, f)==1&&*buf&&*buf!='\r'&&*buf!='\n';i+=1) buf+=1;
	if(i<BUFSZ) {
		*buf=0;
	}
	return i;
}


void* udpserver_routine(file_t f) {
	return NULL;
}

void* tcpserver_routine(file_t f) {
	struct sockaddr_in client;
	socklen_t size;
	pthread_mutex_lock(&f->mutex);
	f->flags|=FISRUNNING;
	pthread_cond_signal(&f->cond);
	pthread_mutex_unlock(&f->mutex);
	/*vm_printf("[VM:DEBUG] Starting I/O thread for %s...\n", f->source);*/
	while(f->flags&FISOPEN) {
		/*vm_printf("i/o wait...\n");*/
		pthread_mutex_lock(&f->mutex);
		while((f->flags&FISOPEN)&&!(f->flags&(FCMDREAD|FCMDWRITE))) {
			pthread_cond_wait(&f->cond, &f->mutex);
		}
		pthread_mutex_unlock(&f->mutex);
		/*vm_printf("In file routine (file %s)... CMD=%X\n", f->source, f->flags&(~FSTATEMASK));*/
		size=sizeof(struct sockaddr_in);
		file_cmd_done(f);
		f->data = (word_t)accept(f->descr.fd, &client, &size);
		f->extra.client.addr=ntohl(client.sin_addr.s_addr);
		f->extra.client.port=ntohs(client.sin_port);
		blocker_resume(_glob_vm, f->blocker);
	}
	/*vm_printf("[VM:DEBUG] Exiting I/O thread for TCP server %s... (flags=%X)\n", f->source, f->flags);*/
	if(f->flags&FCMDREAD) {
		f->data=(word_t)-1;
		file_cmd_done(f);
		blocker_resume(_glob_vm, f->blocker);
	}
	pthread_mutex_lock(&f->mutex);
	f->flags&=~FISRUNNING;
	pthread_cond_signal(&f->cond);
	pthread_mutex_unlock(&f->mutex);
	return NULL;
}

void* file_routine(file_t f) {
	pthread_mutex_lock(&f->mutex);
	f->flags|=FISRUNNING;
	pthread_cond_signal(&f->cond);
	pthread_mutex_unlock(&f->mutex);
	/*vm_printf("[VM:DEBUG] Starting I/O thread for %s...\n", f->source);*/
	while(f->flags&FISOPEN) {
		/*vm_printf("i/o wait...\n");*/
		pthread_mutex_lock(&f->mutex);
		while((f->flags&FISOPEN)&&!(f->flags&(FCMDREAD|FCMDWRITE))) {
			pthread_cond_wait(&f->cond, &f->mutex);
		}
		pthread_mutex_unlock(&f->mutex);
		/*vm_printf("In file routine (file %s)... CMD=%X\n", f->source, f->flags&(~FSTATEMASK));*/
		if(f->flags&FCMDREAD) {
			if(f->flags&FREADABLE) {
				if(f->_overrides[(word_t)f->data_fmt]!=NULL) {
					f->data = f->_overrides[(word_t)f->data_fmt]->unpacker(f, f->data_fmt);
				} else {
					f->data = _unpack(f, f->data_fmt);
				}
			} else {
				vm_fatal("Trying to read from unreadable stream.");
			}
			file_cmd_done(f);
		} else if(f->flags&FCMDWRITE) {
			if(f->flags&FWRITABLE) {
				if(f->_overrides[(word_t)f->data_fmt]!=NULL) {
					f->_overrides[(word_t)f->data_fmt]->packer(f, f->data_fmt, f->data);
				} else {
					_pack(f, f->data_fmt, f->data);
				}
			} else {
				vm_fatal("Trying to write to unwritable stream.");
			}
			file_cmd_done(f);
		}
		if(f->flags&FCMDDONE) {
			blocker_resume(_glob_vm, f->blocker);
		}
	}
	/*vm_printf("[VM:DEBUG] Exiting I/O thread for %s... (flags=%X)\n", f->source, f->flags);*/
	pthread_mutex_lock(&f->mutex);
	f->flags&=~FISRUNNING;
	pthread_cond_signal(&f->cond);
	pthread_mutex_unlock(&f->mutex);
	return NULL;
}

void* dir_routine(file_t f) {
	pthread_mutex_lock(&f->mutex);
	f->flags|=FISRUNNING;
	pthread_cond_signal(&f->cond);
	pthread_mutex_unlock(&f->mutex);
	/*vm_printf("[VM:DEBUG] Starting I/O thread for DIR %s...\n", f->source);*/
	while(f->flags&FISOPEN) {
		/*vm_printf("i/o wait...\n");*/
		pthread_mutex_lock(&f->mutex);
		while((f->flags&FISOPEN)&&!(f->flags&(FCMDREAD|FCMDWRITE))) {
			pthread_cond_wait(&f->cond, &f->mutex);
		}
		pthread_mutex_unlock(&f->mutex);
		/*vm_printf("In file routine (DIR %s)... CMD=%X\n", f->source, f->flags&(~FSTATEMASK));*/
		if(f->flags&FCMDREAD) {
			struct dirent* de = readdir(f->descr.d);
			if(!de) {
				f->data = 0;
			} else {
				f->data = (word_t)vm_string_new(de->d_name);
			}
			file_cmd_done(f);
			blocker_resume(_glob_vm, f->blocker);
		}
	}
	/*vm_printf("[VM:DEBUG] Exiting I/O thread for DIR %s... (flags=%X)\n", f->source, f->flags);*/
	pthread_mutex_lock(&f->mutex);
	f->flags&=~FISRUNNING;
	pthread_cond_signal(&f->cond);
	pthread_mutex_unlock(&f->mutex);
	/*vm_printerrf("[VM:DBG] Done with dir.\n");*/
	return NULL;
}

void file_update_state(file_t f, long flags) {
	word_t oflags=f->flags;
	long ret;
	/* update flags */
	f->flags = flags;
	/* open, close, start/stop thread... */
	if((oflags&FISOPEN) && !(flags&FISOPEN)) {	/* closing file */
		/* close fd */
		/* stop thread */
		/*file_disable_thread(f);*/
		/*oflags &= ~(FISOPEN|FCMDMASK);*/
		/*file_enable_thread(f);*/
		/*vm_printf("SIGNALING THREAD : FILE IS CLOSED.\n");*/
		pthread_mutex_lock(&f->mutex);
		pthread_cond_signal(&f->cond);
		while(f->flags&FISRUNNING) {
			pthread_cond_wait(&f->cond, &f->mutex);
		}
		pthread_mutex_unlock(&f->mutex);
		if(!(oflags&FISSYSTEM)) {
			switch(_file_type(f)) {
			case FISFILE:
			case FISBUFFER:
			case FISSOCKET:
				fclose(f->descr.f);
				break;
			case FISPROCESS:
				/*vm_printerrf("[VM:DBG] closing pipe.\n");*/
				pclose(f->descr.f);
				break;
			case FISDIR:
				closedir(f->descr.d);
				break;
			case FISTCPSERVER:
			case FISUDPSERVER:
				close(f->descr.fd);
				break;
			default:;
				vm_printerrf("[VM:ERR] Undefined file type flags (%X)\n", _file_type(f));
			};
			/*f->descr.fd=-1;*/
		}
		/*vm_printerrf("[VM:DBG] Done closing file.\n");*/
	} else if((flags&FISOPEN) && !(oflags&FISOPEN)) { /* opening file */
		/* start thread */
		pthread_attr_t attr;
		struct sched_param prio;
		/*f->mutex=PTHREAD_MUTEX_INITIALIZER;*/
		pthread_mutex_init(&f->mutex, NULL);
		/*f->cond=PTHREAD_COND_INITIALIZER;*/
		pthread_cond_init(&f->cond, NULL);
		if(pthread_attr_init(&attr)) {
			perror("Error initializing thread attr");
			return;
		}
		if(pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED)) {
			perror("Error setting Detached state");
			return;
		}
		if(pthread_attr_getschedparam(&attr,&prio)) {
			perror("Error getting schedule param struct");
			return;
		}
		prio.sched_priority=sched_get_priority_max(SCHED_FIFO);
		if(pthread_attr_setschedparam(&attr,&prio)) {
			perror("Error setting thread priority");
			return;
		}
		/*file_disable_thread(f);*/
		pthread_mutex_lock(&f->mutex);
		if(file_is_tcpserver(f)) {
			ret = pthread_create(&f->thread, &attr, (void*(*)(void*))tcpserver_routine, f);
		} else if(file_is_udpserver(f)) {
			ret = pthread_create(&f->thread, &attr, (void*(*)(void*))udpserver_routine, f);
		} else if(file_is_dir(f)) {
			ret = pthread_create(&f->thread, &attr, (void*(*)(void*))dir_routine, f);
		} else {
			ret = pthread_create(&f->thread, &attr, (void*(*)(void*))file_routine, f);
		}
		if(ret) {
			perror("Error creating thread");
			return;
		}
		/*vm_printf("Waiting for file thread to run...\n");*/
		f->blocker = blocker_new();
		/*f->rd=f->wr=0;*/
		while(!(f->flags&FISRUNNING)) {
			pthread_cond_wait(&f->cond, &f->mutex);
		}
		pthread_mutex_unlock(&f->mutex);
		/*vm_printf("File thread running !\n");*/
	}
}

void file_deinit(vm_t vm, file_t f) {
	word_t fmt;
	/*vm_printerrf("[VM:DBG] about to reset flags...\n");*/
	file_update_state(f, f->flags&(FTYPEMASK|FISSYSTEM));
	/*vm_printerrf("[VM:DBG] about to free source...\n");*/
	free(f->source);
	/*vm_printerrf("[VM:DBG] about to free overrides...\n");*/
	for(fmt=0;fmt<127;fmt+=1) {
		if(f->_overrides[fmt]) {
			vm_printerrf("[VM:DBG] removing override %p\n", f->_overrides[fmt]);
			free(f->_overrides[fmt]);
		}
	}
	/*vm_printerrf("[VM:DBG] about to free thread blocker...\n");*/
	blocker_free(f->blocker);
	/*vm_printerrf("[VM:DBG] finished deiniting file.\n");*/
}

file_t file_clone(vm_t vm, file_t f) {
	return f;
}



file_t file_new(vm_t vm, const char* source, FILE*f, long flags) {
	int fmt;
	file_t ret = (file_t) vm_obj_new(sizeof(struct _file_t),
			(void (*)(vm_t,void*)) file_deinit,
			(void*(*)(vm_t,void*)) file_clone,
			DataObjUser);
	ret->magic = 0x6106F11E;
	ret->descr.f = f;
	ret->owner = vm->current_thread;
	ret->flags=0;
	for(fmt=0;fmt<127;fmt+=1) {
		ret->_overrides[(word_t)fmt] = NULL;
	}
	ret->source = strdup(source?source:"(unset)");
	file_update_state(ret, flags);
	return ret;
}

file_t dir_new(vm_t vm, const char* source) {
	int fmt;
	file_t ret = (file_t) vm_obj_new(sizeof(struct _file_t),
			(void (*)(vm_t,void*)) file_deinit,
			(void*(*)(vm_t,void*)) file_clone,
			DataObjUser);
	ret->magic = 0x6106D123;
	ret->descr.d = opendir(source);
	ret->owner = vm->current_thread;
	ret->flags=0;
	ret->data=(word_t)-1;
	for(fmt=0;fmt<127;fmt+=1) {
		ret->_overrides[(word_t)fmt] = NULL;
	}
	ret->source = strdup(source?source:"(unset)");
	file_update_state(ret, FREADABLE|FISDIR|FISOPEN);
	return ret;
}

void cmd_pack(vm_t vm, file_t f, char fmt, word_t data) {
	pthread_mutex_lock(&f->mutex);
	file_cmd_write(f);
	f->data_fmt = fmt;
	f->data = data;
	blocker_suspend(vm, f->blocker, vm->current_thread);
	/*file_enable_thread(f);*/
	pthread_cond_signal(&f->cond);
	pthread_mutex_unlock(&f->mutex);
}

void cmd_unpack(vm_t vm, file_t f, char fmt) {
	pthread_mutex_lock(&f->mutex);
	file_cmd_read(f);
	f->data_fmt = fmt;
	blocker_suspend(vm, f->blocker, vm->current_thread);
	/*file_enable_thread(f);*/
	pthread_cond_signal(&f->cond);
	pthread_mutex_unlock(&f->mutex);
}


void file_override_format(file_t f, char fmt, _pack_handler ph, _unpack_handler uh, vm_data_type_t dt, word_t magic) {
	if(f->_overrides[(word_t)fmt]) {
		free(f->_overrides[(word_t)fmt]);
	}
	if(ph&&uh&&dt!=DataNone) {
		format_override_t fo = (format_override_t) malloc(sizeof(struct _format_override));
		fo->packer = ph;
		fo->unpacker = uh;
		fo->data_type = dt;
		fo->magic = magic;
		f->_overrides[(word_t)fmt] = fo;
	}
}

