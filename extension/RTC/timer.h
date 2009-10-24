/*
 * time.h : define Time class
 * manages date maintaining and scheduling of tasks
 *
 */

#ifndef __TIMER_H__
#define __TIMER_H__

/*#include "types.h"*/
#include "vm_types.h"

/** \defgroup Timer High-resolution timer
 * Implements a timer and scheduling of tasks
 * @{
 */

long timer_init();						/**< \brief initialize timer \return 0 on success */
void timer_terminate();						/**< \brief terminate timer \warning all timer_* calls are invalid after this */

void timer_start();						/**< \brief (re)start timer */
void timer_stop();						/**< \brief stop timer */

tinyaml_float_t timer_set_resolution(tinyaml_float_t ms);				/**< \brief set timer resolution expressed in milliseconds (returns actual resolution after setting) */
tinyaml_float_t timer_get_resolution();					/**< \brief get timer resolution expressed in milliseconds */

void timer_set_tempo(tinyaml_float_t bpm);				/**< \brief set global tempo */
tinyaml_float_t timer_get_tempo();					/**< \brief get global tempo */

void timer_set_date(tinyaml_float_t date);				/**< \brief set current date expressed in beats */
tinyaml_float_t timer_get_date();						/**< \brief get current date expressed in beats */
tinyaml_float_t timer_get_seconds();					/**< \brief get current date expressed in seconds */
tinyaml_float_t timer_date_to_seconds(tinyaml_float_t date);			/**< \brief convert \a date from beats units to seconds */


void timer_scheduleTask(tinyaml_float_t date,void*(*task)(tinyaml_float_t,void*),void*task_arg);/**< \brief schedule a task */

void timer_addHandler(void*(*task)(tinyaml_float_t,void*),void*task_arg);	/**< \brief add a permanent timer handler */

void timer_removeHandler(void*(*task)(tinyaml_float_t,void*));			/**< \brief remove a permanent timer handler */

void timer_set_source(tinyaml_float_t(*src_rd)());			/**< \brief set \a src_rd as time source (src_rd returns a date in seconds) */

/* @} */
#endif

