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

//#define __TESTS

#include "timer.h"
#include "list.h"
#include "rtc_alloc.h"
#include "priority_queue.h"

#include <stdio.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>


//#include<pthread.h>
#include "mutex.h"

typedef GenericList list_t;
typedef GenericListNode _node;

typedef _node* node_t;



//list_t perm_handlers;
//list_t tasks;

PQueue tasks;

//pthread_mutex_t mutex;
Mutex mtx;

pthread_cond_t timer_init_done = PTHREAD_COND_INITIALIZER;

/* FIXME : dirty fix to prevent mutex recursion */
long __selfcall=0;

#define LOCK(__x) do { if(!__selfcall) mutexLock(__x); } while(0)
#define UNLOCK(__x) do { if(!__selfcall) mutexUnlock(__x); } while(0)

typedef struct __task_t {
	PQMessage_Fields;
	void*(*task)(tinyaml_float_t,void*);
	void*arg;
}* task_t;

//typedef node_t task_t;

//#define task_task(_node) ((struct __task_t*)_node->data)->task
//#define task_arg(_node) ((struct __task_t*)_node->data)->arg
//#define task_date(_node) ((struct __task_t*)_node->data)->date
#define task_task(_node) _node->task
#define task_arg(_node) _node->arg
#define task_date(_node) _node->date


task_t new_task(tinyaml_float_t date,void*(*task)(tinyaml_float_t,void*),void*arg) {
//	node_t ret=(node_t)malloc(sizeof(struct _node));
	//node_t ret=_alloc(_node);
	task_t ret=_alloc(struct __task_t);
	if(!ret) return NULL;
//	ret->data=malloc(sizeof(struct __task_t));
/*	ret->data=_alloc(struct __task_t);
	if(!ret->data) {
		free(ret);
		return NULL;
	}*/
	task_task(ret)=task;
	task_arg(ret)=arg;
	task_date(ret)=date;
	return ret;
}

void del_task(PQMessage task) {
	//list_remove(task->list,task);
//	listRemove(*(task->list),task);
	if(!task) return;
	//_free(struct __task_t,task->data);
	_free(struct __task_t,task);
	//_free(_node,task);
	//free(task->data);
	//free(task);
}



void timer_scheduleTask(tinyaml_float_t date,void*(*task)(tinyaml_float_t,void*),void*task_arg) {
	task_t t;
	LOCK(mtx);
//	printf("hop\n");
	t=new_task(date,task,task_arg);
//	printf("hip\n");
	t->date=date;
	//pthread_mutex_lock(&mutex);
	pqEnqueue(tasks,date,(PQMessage)t);
//	printf("hep\n");
	UNLOCK(mtx);
}


/*
void timer_addHandler(void*(*task)(tinyaml_float_t,void*),void*task_arg) {
	task_t newt=new_task(0,task,task_arg);
	//list_insertAfter(&perm_handlers,perm_handlers.tail,newt);
	//pthread_mutex_lock(&mutex);
	listAddTail(perm_handlers,newt);
	//pthread_mutex_unlock(&mutex);
}

void timer_removeHandler(void*(*task)(tinyaml_float_t,void*)) {
	task_t t=perm_handlers.head;
	while(t&&task_task(t)!=task)
		t=t->next;
	if(t) {
		//list_remove(t->list,t);
		//pthread_mutex_lock(&mutex);
		listRemove(*(t->list),t);
		//pthread_mutex_unlock(&mutex);
	}
}
*/

#define N_RES 7

tinyaml_float_t allowed_resolutions[N_RES]= {	1/.128,	1/.256,	1/.512,	1/1.024,1/2.048,1/4.096,1/8.192 };
unsigned long res_to_rtc_freq[N_RES]= {	128,	256,	512,	1024,	2048,	4096,	8192 };

volatile long rtc_fd;
static pthread_t rtc_thread;
volatile long timer_is_running=0;

tinyaml_float_t(*ext_source_read)();

volatile double timer_resolution;
volatile tinyaml_float_t  timer_date;
volatile tinyaml_float_t  timer_tempo;
volatile double timer_dbeat;
volatile tinyaml_float_t  timer_basedate;

void*timer_routine(void*arg);

#ifdef __TESTS
volatile long max_pileup;
volatile tinyaml_float_t average_pileup;
volatile long n_pileups;
volatile long n_ticks;
volatile long n_realticks;
#define PILEUP 10
#endif




long timer_init() {
	pthread_attr_t attr;
	struct sched_param prio;
	/*long err;*/

//	listInit(perm_handlers);
//	listInit(tasks);
	tasks=pqCreate(1.,3,18,del_task);

	mutexInit(mtx);
//	pthread_mutex_init(&mutex,NULL);
//	if( (err = pthread_mutex_trylock( &mutex )) != 0 )
//		printf("trylock => %i\n",err);
//	if((err=pthread_mutex_unlock(&mutex))) {
//		printf("unlock => %i\n",err);
//	}
	//list_init(&perm_handlers);
	//list_init(&tasks);

	if(pthread_attr_init(&attr)) {
		perror("Error initializing thread struct");
		return 1;
	}
	if(pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED)) {
		perror("Error setting Detached state");
		return 1;
	}
	if(pthread_attr_setschedpolicy(&attr,SCHED_FIFO)) {
		perror("Error setting realtime scheduler policy");
		return 1;
	}
	if(pthread_attr_getschedparam(&attr,&prio)) {
		perror("Error getting schedule param struct");
		return 1;
	}
	prio.sched_priority=sched_get_priority_max(SCHED_FIFO);
	if(pthread_attr_setschedparam(&attr,&prio)) {
		perror("Error setting thread priority");
		return 1;
	}

	/*timer_is_running=0;*/

	if(pthread_create(&rtc_thread,&attr,timer_routine,NULL)) {
		perror("Error creating thread");
		return 1;
	}

#ifdef __TESTS
	max_pileup=0;
#endif
//	sleep(2);

	/*while(!timer_is_running) sleep(0);*/

	while(!timer_is_running) {
		mutexLock(mtx);
		pthread_cond_wait(&timer_init_done, &mtx);
		mutexUnlock(mtx);
	}

	if(timer_is_running==-1) {
		return 1;
	}

	return 0;
}


void timer_terminate() {
	timer_is_running=0;
	timer_start();				/* to be sure the flag will be read */
	/*while(!timer_is_running) sleep(0);*/
	pthread_cond_wait(&timer_init_done,&mtx);
	timer_is_running=0;
	pqDestroy(tasks);
}

void timer_set_date(tinyaml_float_t date) {
	LOCK(mtx);
	//pthread_mutex_lock(&mutex);
	timer_date=date;
	timer_basedate=date*60.f/timer_tempo;
	UNLOCK(mtx);
	//pthread_mutex_unlock(&mutex);
}

tinyaml_float_t timer_get_date() {
	return timer_date;
}

void timer_set_tempo(tinyaml_float_t bpm) {
	if(!bpm) return;
	LOCK(mtx);
//	//pthread_mutex_lock(&mutex);
	timer_basedate=timer_get_seconds();
	timer_tempo=bpm;
	timer_dbeat=timer_resolution*bpm*.001/60.;
	UNLOCK(mtx);
//	//pthread_mutex_unlock(&mutex);
}

tinyaml_float_t timer_get_tempo() {
	return timer_tempo;
}

tinyaml_float_t timer_get_seconds() {
	return timer_basedate+timer_date*60.f/timer_tempo;
}

tinyaml_float_t timer_set_resolution(tinyaml_float_t ms) {
	long scbak=__selfcall;
	long i=0;
	LOCK(mtx);
	while(i<(N_RES-1)&&ms<allowed_resolutions[i]) ++i;

	//pthread_mutex_lock(&mutex);
	timer_resolution=allowed_resolutions[i];
	__selfcall=1;
	timer_set_tempo(timer_tempo);	/* update dbeat */
	__selfcall=scbak;
	ioctl(rtc_fd,RTC_IRQP_SET,res_to_rtc_freq[i]);
	//pthread_mutex_unlock(&mutex);
	UNLOCK(mtx);
	return timer_resolution;
}


tinyaml_float_t timer_get_resolution() {
	return timer_resolution;
}


void timer_set_source(tinyaml_float_t(*src_rd)()) {
	//pthread_mutex_lock(&mutex);
	LOCK(mtx);
	ext_source_read=src_rd;
	UNLOCK(mtx);
	//pthread_mutex_unlock(&mutex);
}


void timer_start() {
	ioctl (rtc_fd, RTC_PIE_ON, 0);
}


void timer_stop() {
	ioctl (rtc_fd, RTC_PIE_OFF, 0);
}


void*timer_routine(void*arg) {
	long ret;
#ifdef __TESTS
	register long n;
#endif
	unsigned long data;
	task_t task;
	rtc_fd=open("/dev/rtc",O_RDONLY,0);
	if(rtc_fd==-1) {
		/* FIXME handle error */
		perror("Error initializing rtc timer");
		timer_is_running=-1;
		return NULL;
	}
	ioctl (rtc_fd, RTC_AIE_OFF, 0);
	ioctl (rtc_fd, RTC_WIE_OFF, 0);
	ioctl (rtc_fd, RTC_UIE_OFF, 0);
	ioctl (rtc_fd, RTC_PIE_OFF, 0);
	timer_basedate=0;
	timer_date=0;
	timer_tempo=60;
	timer_set_tempo(60);
	timer_set_resolution(10);

	/*fflush(stdout);*/
	
	ioctl (rtc_fd, RTC_PIE_ON, 0);
	ret=read(rtc_fd,&data,sizeof(unsigned long));
#ifdef __TESTS
	max_pileup=0;
	average_pileup=0;
	n=0;
	n_pileups=0;
	n_ticks=0;
#endif

	mutexLock(mtx);
	timer_is_running=1;
	pthread_cond_signal(&timer_init_done);
	mutexUnlock(mtx);

	/*printf("Timer now runs\n"); fflush(stdout);*/
	while(timer_is_running) {
		ret=read(rtc_fd,&data,sizeof(unsigned long));
		mutexLock(mtx);
//		printf("%lX\n",data);
		data>>=8;			/* mask out irq bits */

		if(ext_source_read) {
			timer_date=ext_source_read();
		} else {		
			timer_date+=timer_dbeat*data;
		}

		//pthread_mutex_lock(&mutex);

//		printf("timer_date %f\n",timer_date);
		__selfcall=1;
		while( timer_is_running && (task=(task_t)pqDequeue(tasks,timer_date)) ) {
			if(task_task(task)) task_task(task) ( task_date(task), task_arg(task) );
			del_task((PQMessage)task);
		}
		__selfcall=0;
		
//		task=tasks.head;
//		while(task&&task_date(task)<=timer_date) {
//			task_task(task) ( task_date(task), task_arg(task) );
//			tmp=task;
//			task=task->next;
//			del_task(tmp);
//		}

#ifdef __TESTS
		if(data>max_pileup) max_pileup=data;
		average_pileup=(tinyaml_float_t)(average_pileup*n+data)/(tinyaml_float_t)(n+1);
		++n;
		if(data>PILEUP) ++n_pileups;
		n_ticks+=data;
#endif
		mutexUnlock(mtx);
		//pthread_mutex_unlock(&mutex);
	}

	close(rtc_fd);
	rtc_fd=0;
#ifdef __TESTS
	n_realticks=n;
	printf("timer is now terminated.\nmax_pileup=%i average_pileup=%f (%li pileups in %li ticks (%li real))\n",max_pileup,average_pileup,n_pileups,n_ticks,n_realticks);
#endif
	mutexLock(mtx);
	timer_is_running=-1;
	pthread_cond_signal(&timer_init_done);
	mutexUnlock(mtx);
	return NULL;
}



