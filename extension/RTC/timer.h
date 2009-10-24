/*
 * time.h : define Time class
 * manages date maintaining and scheduling of tasks
 *
 */

#ifndef __TIMER_H__
#define __TIMER_H__

/*#include "types.h"*/

/** \defgroup Timer High-resolution timer
 * Implements a timer and scheduling of tasks
 * @{
 */

long timer_init();						/**< \brief initialize timer \return 0 on success */
void timer_terminate();						/**< \brief terminate timer \warning all timer_* calls are invalid after this */

void timer_start();						/**< \brief (re)start timer */
void timer_stop();						/**< \brief stop timer */

float timer_set_resolution(float ms);				/**< \brief set timer resolution expressed in milliseconds (returns actual resolution after setting) */
float timer_get_resolution();					/**< \brief get timer resolution expressed in milliseconds */

void timer_set_tempo(float bpm);				/**< \brief set global tempo */
float timer_get_tempo();					/**< \brief get global tempo */

void timer_set_date(float date);				/**< \brief set current date expressed in beats */
float timer_get_date();						/**< \brief get current date expressed in beats */
float timer_get_seconds();					/**< \brief get current date expressed in seconds */
float timer_date_to_seconds(float date);			/**< \brief convert \a date from beats units to seconds */


void timer_scheduleTask(float date,void*(*task)(float,void*),void*task_arg);/**< \brief schedule a task */

void timer_addHandler(void*(*task)(float,void*),void*task_arg);	/**< \brief add a permanent timer handler */

void timer_removeHandler(void*(*task)(float,void*));			/**< \brief remove a permanent timer handler */

void timer_set_source(float(*src_rd)());			/**< \brief set \a src_rd as time source (src_rd returns a date in seconds) */

/* @} */
#endif

