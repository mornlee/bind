/*
 * Copyright (C) 1998, 1999  Internet Software Consortium.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include	<config.h>

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>

#include	<isc/assertions.h>
#include	<isc/condition.h>
#include	<isc/mutex.h>
#include	<isc/error.h>
#include	<isc/mem.h>
#include	<isc/task.h>
#include	<isc/thread.h>
#include	<isc/result.h>
#include	<isc/timer.h>

#include	<tests/t_api.h>

void	t1(void);
void	t2(void);
void	t3(void);
void	t4(void);
void	t7(void);
void	t10(void);
void	t11(void);
void	t12(void);
void	t13(void);

static int	t_tasks1(void);
static int	t_tasks2(void);
static int	t_tasks3(void);
static int	t_tasks4(void);
static int	t_tasks7(void);
static int	t_tasks10(void);
static int	t_tasks11(int purgable);
static int	t_tasks12(void);
static int	t_tasks13(void);

isc_mem_t *mctx = NULL;

static void
t1_callback(isc_task_t *task, isc_event_t *event)
{
	int	i;
	int	j;

	j = 0;
	task = task;
	for (i = 0; i < 1000000; i++)
		j += 100;

	t_info("task %s\n", event->arg);
	isc_event_free(&event);
}

static void
t1_shutdown(isc_task_t *task, isc_event_t *event) {

	task = task;
	t_info("shutdown %s\n", event->arg);
	isc_event_free(&event);
}

static void
my_tick(isc_task_t *task, isc_event_t *event)
{
	task = task;
	t_info("%s\n", event->arg);
	isc_event_free(&event);
}

/*
 * Adapted from RTH's original task_test program
 */

static int
t_tasks1() {
	char			*p;
	isc_taskmgr_t		*manager;
	isc_task_t		*task1;
	isc_task_t		*task2;
	isc_task_t		*task3;
	isc_task_t		*task4;
	isc_event_t		*event;
	unsigned int		workers;
	isc_timermgr_t		*timgr;
	isc_timer_t		*ti1;
	isc_timer_t		*ti2;
	isc_result_t		isc_result;
	struct isc_time		absolute;
	struct isc_interval	interval;

	manager = NULL;
	task1 = NULL;
	task2 = NULL;
	task3 = NULL;
	task4 = NULL;

	workers = 2;
	p = t_getenv("ISC_TASK_WORKERS");
	if (p != NULL)
		workers = atoi(p);
	if (workers < 1) {
		t_info("Bad config value for ISC_TASK_WORKERS, %d\n", workers);
		return(T_UNRESOLVED);
	}

	isc_result = isc_mem_create(0, 0, &mctx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mem_create failed %d\n", isc_result);
		return(T_UNRESOLVED);
	}

	isc_result = isc_taskmgr_create(mctx, workers, 0, &manager);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_taskmgr_create failed %d\n", isc_result);
		return(T_FAIL);
	}

	isc_result = isc_task_create(manager, NULL, 0, &task1);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_create failed %d\n", isc_result);
		return(T_FAIL);
	}

	isc_result = isc_task_create(manager, NULL, 0, &task2);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_create failed %d\n", isc_result);
		return(T_FAIL);
	}

	isc_result = isc_task_create(manager, NULL, 0, &task3);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_create failed %d\n", isc_result);
		return(T_FAIL);
	}

	isc_result = isc_task_create(manager, NULL, 0, &task4);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_create failed %d\n", isc_result);
		return(T_FAIL);
	}

	isc_result = isc_task_onshutdown(task1, t1_shutdown, "1");
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_onshutdown failed %d\n", isc_result);
		return(T_FAIL);
	}

	isc_result = isc_task_onshutdown(task2, t1_shutdown, "2");
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_onshutdown failed %d\n", isc_result);
		return(T_FAIL);
	}

	isc_result = isc_task_onshutdown(task3, t1_shutdown, "3");
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_onshutdown failed %d\n", isc_result);
		return(T_FAIL);
	}

	isc_result = isc_task_onshutdown(task4, t1_shutdown, "4");
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_onshutdown failed %d\n", isc_result);
		return(T_FAIL);
	}

	timgr = NULL;
	isc_result = isc_timermgr_create(mctx, &timgr);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_timermgr_create %d\n", isc_result);
		return(T_UNRESOLVED);
	}

	ti1 = NULL;
	isc_time_settoepoch(&absolute);
	isc_interval_set(&interval, 1, 0);
	isc_result = isc_timer_create(timgr, isc_timertype_ticker,
				&absolute, &interval,
				task1, my_tick, "tick", &ti1);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_timer_create %d\n", isc_result);
		return(T_UNRESOLVED);
	}

	ti2 = NULL;
	isc_time_settoepoch(&absolute);
	isc_interval_set(&interval, 1, 0);
	isc_result = isc_timer_create(timgr, isc_timertype_ticker,
				       &absolute, &interval,
				       task2, my_tick, "tock", &ti2);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_timer_create %d\n", isc_result);
		return(T_UNRESOLVED);
	}

	
	sleep(2);

	/*
	 * Note:  (void *)1 is used as a sender here, since some compilers
	 * don't like casting a function pointer to a (void *).
	 *
	 * In a real use, it is more likely the sender would be a
	 * structure (socket, timer, task, etc) but this is just a test
	 * program.
	 */
	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "1",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task1, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "1",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task1, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "1",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task1, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "1",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task1, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "1",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task1, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "1",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task1, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "1",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task1, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "1",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task1, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "1",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task1, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "2",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task2, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "3",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task3, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "4",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task4, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "2",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task2, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "3",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task3, &event);

	event = isc_event_allocate(mctx, (void *)1, 1, t1_callback, "4",
				   sizeof *event);
	if (event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_task_send(task4, &event);

	isc_task_purge(task3, NULL, 0, 0);

	isc_task_detach(&task1);
	isc_task_detach(&task2);
	isc_task_detach(&task3);
	isc_task_detach(&task4);

	sleep(10);
	isc_timer_detach(&ti1);
	isc_timer_detach(&ti2);
	isc_timermgr_destroy(&timgr);
	isc_taskmgr_destroy(&manager);
	
	isc_mem_destroy(&mctx);
	return(T_PASS);
}

static char	*a1 =	"The task subsystem can create and manage tasks";

void
t1() {
	int	result;

	t_assert("tasks", 1, T_REQUIRED, a1);
	result = t_tasks1();
	t_result(result);
}

#define			T2_NTASKS	10000

static isc_event_t	*T2_event;
static isc_taskmgr_t	*T2_manager;
static isc_mem_t	*T2_mctx;
static isc_condition_t	T2_cv;
static isc_mutex_t	T2_mx;
static int		T2_done;
static int		T2_nprobs;
static int		T2_nfails;
static int		T2_ntasks;

static void
t2_shutdown(isc_task_t *task, isc_event_t *event) {

	isc_result_t	isc_result;

	task = task; /* notused */

	if (event->arg != NULL) {
		isc_task_destroy((isc_task_t**) &event->arg);
	}
	else {
		isc_result = isc_mutex_lock(&T2_mx);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_mutex_lock failed %d\n", isc_result);
			++T2_nprobs;
		}
	
		T2_done = 1;

		isc_result = isc_condition_signal(&T2_cv);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_condition_signal failed %d\n", isc_result);
			++T2_nprobs;
		}
	
		isc_result = isc_mutex_unlock(&T2_mx);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_mutex_unlock failed %d\n", isc_result);
			++T2_nprobs;
		}

		isc_event_free(&T2_event);
		isc_taskmgr_destroy(&T2_manager);
		isc_mem_destroy(&T2_mctx);
	}
}

static void
t2_callback(isc_task_t *task, isc_event_t *event)
{
	isc_result_t	isc_result;
	isc_task_t	*newtask;

	++T2_ntasks;

	if (T_debug && ((T2_ntasks % 100) == 0)) {
		t_info("T2_ntasks %d\n", T2_ntasks);
	}

	if (event->arg) {

		event->arg = (void* ) (((int) event->arg) - 1);

		/* create a new task and forward the message */
		newtask = NULL;
		isc_result = isc_task_create(T2_manager, NULL, 0, &newtask);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_task_create failed %d\n", isc_result);
			++T2_nfails;
			return;
		}
	
		isc_result = isc_task_onshutdown(newtask, t2_shutdown,
							(void *) task);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_task_onshutdown failed %d\n",
						isc_result);
			++T2_nfails;
			return;
		}
	
		isc_task_send(newtask, &event);
	}
	else {
		/* time to unwind, shutdown should perc back up */
		isc_task_destroy(&task);
	}
}

static int
t_tasks2(void) {

	int			ntasks;
	int			result;
	char			*p;
	isc_event_t		*event;
	unsigned int		workers;
	isc_result_t		isc_result;


	T2_manager = NULL;
	T2_done = 0;
	T2_nprobs = 0;
	T2_nfails = 0;
	T2_ntasks = 0;

	workers = 2;
	p = t_getenv("ISC_TASK_WORKERS");
	if (p != NULL)
		workers = atoi(p);
	if (workers < 1) {
		t_info("Bad config value for ISC_TASK_WORKERS, %d\n", workers);
		return(T_UNRESOLVED);
	}

	p = t_getenv("ISC_TASKS_MIN");
	if (p != NULL)
		ntasks = atoi(p);
	else
		ntasks = T2_NTASKS;
	if (ntasks == 0) {
		t_info("Bad config value for ISC_TASKS_MIN, %d\n", ntasks);
		return(T_UNRESOLVED);
	}

	t_info("Testing with %d tasks\n", ntasks);

	isc_result = isc_mutex_init(&T2_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_init failed %d\n", isc_result);
		return(T_UNRESOLVED);
	}

	isc_result = isc_condition_init(&T2_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_condition_init failed %d\n", isc_result);
		return(T_UNRESOLVED);
	}

	isc_result = isc_mem_create(0, 0, &T2_mctx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mem_create failed %d\n", isc_result);
		return(T_UNRESOLVED);
	}

	isc_result = isc_taskmgr_create(T2_mctx, workers, 0, &T2_manager);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_taskmgr_create failed %d\n", isc_result);
		return(T_FAIL);
	}

	T2_event = isc_event_allocate(T2_mctx, (void *)1, 1, t2_callback,
					(void *) ntasks, sizeof *event);
	if (T2_event == NULL) {
		t_info("isc_event_allocate failed\n");
		return(T_UNRESOLVED);
	}

	isc_result = isc_mutex_lock(&T2_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %d\n", isc_result);
		return(T_UNRESOLVED);
	}

	t2_callback(NULL, T2_event);

	while (T2_done == 0) {
		isc_result = isc_condition_wait(&T2_cv, &T2_mx);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_condition_wait failed %d\n", isc_result);
			return(T_UNRESOLVED);
		}
	}

	result = T_UNRESOLVED;

	if ((T2_nfails == 0) && (T2_nprobs == 0))
		result = T_PASS;
	else if (T2_nfails != 0)
		result = T_FAIL;

	return(result);
}

static char	*a2 = "The task subsystem can create ISC_TASKS_MIN tasks";

void
t2() {
	int	result;

	t_assert("tasks", 2, T_REQUIRED, a2);
	result = t_tasks2();
	t_result(result);
}

#define	T3_NEVENTS	256

static	int		T3_flag;
static	int		T3_nevents;
static	int		T3_nsdevents;
static	isc_mutex_t	T3_mx;
static	isc_condition_t	T3_cv;
static	int		T3_nfails;
static	int		T3_nprobs;

static void
t3_sde1(isc_task_t *task, isc_event_t *event) {

	task = task;

	if (T3_nevents != T3_NEVENTS) {
		t_info("Some events were not processed\n");
		++T3_nprobs;
	}
	if (T3_nsdevents == 1) {
		++T3_nsdevents;
	}
	else {
		t_info("Shutdown events not processed in LIFO order\n");
		++T3_nfails;
	}
	isc_event_free(&event);
}

static void
t3_sde2(isc_task_t *task, isc_event_t *event) {

	task = task;

	if (T3_nevents != T3_NEVENTS) {
		t_info("Some events were not processed\n");
		++T3_nprobs;
	}
	if (T3_nsdevents == 0) {
		++T3_nsdevents;
	}
	else {
		t_info("Shutdown events not processed in LIFO order\n");
		++T3_nfails;
	}
	isc_event_free(&event);
}

static void
t3_event1(isc_task_t *task, isc_event_t *event) {

	isc_result_t	isc_result;

	task = task;

	isc_result = isc_mutex_lock(&T3_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %s\n",
				isc_result_totext(isc_result));
		++T3_nprobs;
	}
	while (T3_flag != 1) {
		(void) isc_condition_wait(&T3_cv, &T3_mx);
	}

	isc_result = isc_mutex_unlock(&T3_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_unlock failed %s\n",
				isc_result_totext(isc_result));
		++T3_nprobs;
	}
	isc_event_free(&event);
}

static void
t3_event2(isc_task_t *task, isc_event_t *event) {

	task = task;

	++T3_nevents;
	isc_event_free(&event);
}

static int
t_tasks3() {

	int		cnt;
	int		result;
	char		*p;
	isc_mem_t	*mctx;
	isc_taskmgr_t	*tmgr;
	isc_task_t	*task;
	unsigned int	workers;
	isc_event_t	*event;
	isc_result_t	isc_result;
	void		*sender;
	isc_eventtype_t	event_type;

	T3_flag = 0;
	T3_nevents = 0;
	T3_nsdevents = 0;
	T3_nfails = 0;
	T3_nprobs = 0;

	sender = (void *) 1;
	event_type = 3;

	workers = 2;
	p = t_getenv("ISC_TASK_WORKERS");
	if (p != NULL)
		workers = atoi(p);

	mctx = NULL;
	isc_result = isc_mem_create(0, 0, &mctx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mem_create failed %s\n",
				isc_result_totext(isc_result));
		return(T_UNRESOLVED);
	}

	isc_result = isc_mutex_init(&T3_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_init failed %s\n",
				isc_result_totext(isc_result));
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	isc_result = isc_condition_init(&T3_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_condition_init failed %s\n",
				isc_result_totext(isc_result));
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	tmgr = NULL;
	isc_result = isc_taskmgr_create(mctx, workers, 0, &tmgr);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_taskmgr_create failed %s\n",
				isc_result_totext(isc_result));
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	isc_result = isc_mutex_lock(&T3_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %s\n",
				isc_result_totext(isc_result));
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	task = NULL;
	isc_result = isc_task_create(tmgr, mctx, 0, &task);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_create failed %s\n",
				isc_result_totext(isc_result));
		isc_mutex_unlock(&T3_mx);
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	/* this event causes the task to wait on T3_cv */
	event = isc_event_allocate(mctx, sender, event_type, t3_event1, NULL,
				   sizeof(*event));
	isc_task_send(task, &event);

	/* now we fill up the task's event queue with some events */
	for (cnt = 0; cnt < T3_NEVENTS; ++cnt) {
		event = isc_event_allocate(mctx, sender, event_type,
					   t3_event2, NULL, sizeof(*event));
		isc_task_send(task, &event);
	}

	/* now we register two shutdown events */
	isc_result = isc_task_onshutdown(task, t3_sde1, NULL);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_send failed %s\n",
				isc_result_totext(isc_result));
		isc_mutex_unlock(&T3_mx);
		isc_task_destroy(&task);
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	isc_result = isc_task_onshutdown(task, t3_sde2, NULL);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_send failed %s\n",
				isc_result_totext(isc_result));
		isc_mutex_unlock(&T3_mx);
		isc_task_destroy(&task);
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	isc_task_shutdown(task);

	/* now we free the task by signaling T3_cv */
	T3_flag = 1;
	isc_result = isc_condition_signal(&T3_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_send failed %s\n",
				isc_result_totext(isc_result));
		++T3_nprobs;
	}

	isc_result = isc_mutex_unlock(&T3_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_send failed %s\n",
				isc_result_totext(isc_result));
		++T3_nprobs;
	}
	

	isc_task_detach(&task);
	isc_taskmgr_destroy(&tmgr);
	isc_mem_destroy(&mctx);

	if (T3_nsdevents != 2) {
		t_info("T3_nsdevents == %d, expected 2\n", T3_nsdevents);
		++T3_nfails;
	}

	if (T3_nevents != T3_nevents) {
		t_info("T3_nevents == %d, expected 2\n", T3_nevents);
		++T3_nfails;
	}

	result = T_UNRESOLVED;

	if (T3_nfails != 0)
		result = T_FAIL;
	else if ((T3_nfails == 0) && (T3_nprobs == 0))
		result = T_PASS;

	return(result);
}

static char	*a3 =	"When isc_task_shutdown() is called, any shutdown events "
			"that have been requested via prior isc_task_onshutdown() calls "
			"are posted in LIFO order.";
void
t3(void) {
	int	result;

	t_assert("tasks", 3, T_REQUIRED, a3);
	result = t_tasks3();
	t_result(result);
}

static isc_mutex_t	T4_mx;
static isc_condition_t	T4_cv;
static int		T4_flag;
static int		T4_nprobs;
static int		T4_nfails;

static void
t4_event1(isc_task_t *task, isc_event_t *event) {

	isc_result_t	isc_result;

	task = task;

	isc_result = isc_mutex_lock(&T4_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %s\n",
				isc_result_totext(isc_result));
		++T4_nprobs;
	}
	while (T4_flag != 1) {
		(void) isc_condition_wait(&T4_cv, &T4_mx);
	}

	isc_result = isc_mutex_unlock(&T4_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_unlock failed %s\n",
				isc_result_totext(isc_result));
		++T4_nprobs;
	}
	isc_event_free(&event);
}

static void
t4_sde(isc_task_t *task, isc_event_t *event) {

	task = task;

	/* no-op */
	isc_event_free(&event);
}

static int
t_tasks4() {
	int		result;
	char		*p;
	isc_mem_t	*mctx;
	isc_taskmgr_t	*tmgr;
	isc_task_t	*task;
	unsigned int	workers;
	isc_result_t	isc_result;
	void		*sender;
	isc_eventtype_t	event_type;
	isc_event_t	*event;


	T4_nprobs = 0;
	T4_nfails = 0;
	T4_flag = 0;

	result = T_UNRESOLVED;
	sender = (void *) 1;
	event_type = 4;

	workers = 2;
	p = t_getenv("ISC_TASK_WORKERS");
	if (p != NULL)
		workers = atoi(p);

	mctx = NULL;
	isc_result = isc_mem_create(0, 0, &mctx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mem_create failed %s\n",
				isc_result_totext(isc_result));
		return(T_UNRESOLVED);
	}

	isc_result = isc_mutex_init(&T4_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_init failed %s\n",
				isc_result_totext(isc_result));
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	isc_result = isc_condition_init(&T4_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_condition_init failed %s\n",
				isc_result_totext(isc_result));
		isc_mutex_destroy(&T4_mx);
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	tmgr = NULL;
	isc_result = isc_taskmgr_create(mctx, workers, 0, &tmgr);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_taskmgr_create failed %s\n",
				isc_result_totext(isc_result));
		isc_mutex_destroy(&T4_mx);
		isc_condition_destroy(&T4_cv);
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	isc_result = isc_mutex_lock(&T4_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %s\n",
				isc_result_totext(isc_result));
		isc_mutex_destroy(&T4_mx);
		isc_condition_destroy(&T4_cv);
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	task = NULL;
	isc_result = isc_task_create(tmgr, mctx, 0, &task);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_create failed %s\n",
				isc_result_totext(isc_result));
		isc_mutex_destroy(&T4_mx);
		isc_condition_destroy(&T4_cv);
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	/* this event causes the task to wait on T4_cv */
	event = isc_event_allocate(mctx, sender, event_type, t4_event1, NULL,
				   sizeof(*event));
	isc_task_send(task, &event);

	isc_task_shutdown(task);

	isc_result = isc_task_onshutdown(task, t4_sde, NULL);
	if (isc_result != ISC_R_SHUTTINGDOWN) {
		t_info("isc_task_onshutdown returned %s\n",
				isc_result_totext(isc_result));
		++T4_nfails;
	}

	/* release the task */
	T4_flag = 1;

	isc_result = isc_condition_signal(&T4_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_condition_signal failed %s\n",
				isc_result_totext(isc_result));
		++T4_nprobs;
	}

	isc_result = isc_mutex_unlock(&T4_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_unlock failed %s\n",
				isc_result_totext(isc_result));
		++T4_nprobs;
	}

	isc_mutex_destroy(&T4_mx);
	isc_condition_destroy(&T4_cv);
	isc_task_detach(&task);
	isc_taskmgr_destroy(&tmgr);
	isc_mem_destroy(&mctx);

	result = T_UNRESOLVED;

	if (T4_nfails != 0)
		result = T_FAIL;
	else if ((T4_nfails == 0) && (T4_nprobs == 0))
		result = T_PASS;

	return(result);
}

static char	*a4 =	"After isc_task_shutdown() has been called, any call to "
			"isc_task_onshutdown() will return ISC_R_SHUTTINGDOWN.";

void
t4() {
	int	result;

	t_assert("tasks", 4, T_REQUIRED, a4);
	result = t_tasks4();
	t_result(result);
}

static int		T7_nprobs;
static int		T7_nfails;
static int		T7_eflag;
static int		T7_sdflag;
static isc_mutex_t	T7_mx;
static isc_condition_t	T7_cv;

static void
t7_event1(isc_task_t *task, isc_event_t *event) {

	task = task;

	++T7_eflag;

	isc_event_free(&event);
}

static void
t7_sde(isc_task_t *task, isc_event_t *event) {

	isc_result_t	isc_result;

	task = task;

	isc_result = isc_mutex_lock(&T7_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %s\n",
				isc_result_totext(isc_result));
		++T7_nprobs;
	}

	++T7_sdflag;

	isc_result = isc_condition_signal(&T7_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_condition_signal failed %s\n",
				isc_result_totext(isc_result));
		++T7_nprobs;
	}

	isc_result = isc_mutex_unlock(&T7_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_unlock failed %s\n",
				isc_result_totext(isc_result));
		++T7_nprobs;
	}

	isc_event_free(&event);
}

static int
t_tasks7() {
	int		result;
	char		*p;
	isc_mem_t	*mctx;
	isc_taskmgr_t	*tmgr;
	isc_task_t	*task;
	unsigned int	workers;
	isc_result_t	isc_result;
	void		*sender;
	isc_eventtype_t	event_type;
	isc_event_t	*event;
	isc_time_t	now;
	isc_interval_t	interval;


	T7_nprobs = 0;
	T7_nfails = 0;
	T7_sdflag = 0;
	T7_eflag = 0;

	result = T_UNRESOLVED;
	sender = (void *) 1;
	event_type = 7;

	workers = 2;
	p = t_getenv("ISC_TASK_WORKERS");
	if (p != NULL)
		workers = atoi(p);

	mctx = NULL;
	isc_result = isc_mem_create(0, 0, &mctx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mem_create failed %s\n",
				isc_result_totext(isc_result));
		return(T_UNRESOLVED);
	}

	isc_result = isc_mutex_init(&T7_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_init failed %s\n",
				isc_result_totext(isc_result));
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	isc_result = isc_condition_init(&T7_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_condition_init failed %s\n",
				isc_result_totext(isc_result));
		isc_mutex_destroy(&T7_mx);
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	tmgr = NULL;
	isc_result = isc_taskmgr_create(mctx, workers, 0, &tmgr);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_taskmgr_create failed %s\n",
				isc_result_totext(isc_result));
		isc_mutex_destroy(&T7_mx);
		isc_condition_destroy(&T7_cv);
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	isc_result = isc_mutex_lock(&T7_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %s\n",
				isc_result_totext(isc_result));
		isc_mutex_destroy(&T7_mx);
		isc_condition_destroy(&T7_cv);
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		return(T_FAIL);
	}

	task = NULL;
	isc_result = isc_task_create(tmgr, mctx, 0, &task);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_create failed %s\n",
				isc_result_totext(isc_result));
		isc_mutex_destroy(&T7_mx);
		isc_condition_destroy(&T7_cv);
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		return(T_FAIL);
	}

	isc_result = isc_task_onshutdown(task, t7_sde, NULL);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_onshutdown returned %s\n",
				isc_result_totext(isc_result));
		isc_mutex_destroy(&T7_mx);
		isc_condition_destroy(&T7_cv);
		isc_task_destroy(&task);
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	event = isc_event_allocate(mctx, sender, event_type, t7_event1, NULL,
				   sizeof(*event));
	isc_task_send(task, &event);

	isc_task_shutdown(task);

	interval.seconds = 5;
	interval.nanoseconds = 0;

	while (T7_sdflag == 0) {
		isc_result = isc_time_nowplusinterval(&now, &interval);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_time_nowplusinterval failed %s\n",
					isc_result_totext(isc_result));
			isc_mutex_destroy(&T7_mx);
			isc_condition_destroy(&T7_cv);
			isc_task_destroy(&task);
			isc_taskmgr_destroy(&tmgr);
			isc_mem_destroy(&mctx);
			return(T_UNRESOLVED);
		}
	
		isc_result = isc_condition_waituntil(&T7_cv, &T7_mx, &now);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_condition_waituntil returned %s\n",
					isc_result_totext(isc_result));
			isc_mutex_destroy(&T7_mx);
			isc_condition_destroy(&T7_cv);
			isc_task_destroy(&task);
			isc_taskmgr_destroy(&tmgr);
			isc_mem_destroy(&mctx);
			return(T_FAIL);
		}
	}

	isc_result = isc_mutex_unlock(&T7_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_unlock failed %s\n",
				isc_result_totext(isc_result));
		++T7_nprobs;
	}

	isc_mutex_destroy(&T7_mx);
	isc_condition_destroy(&T7_cv);
	isc_task_detach(&task);
	isc_taskmgr_destroy(&tmgr);
	isc_mem_destroy(&mctx);

	result = T_UNRESOLVED;

	if (T7_eflag == 0)
		++T7_nfails;

	if (T7_nfails != 0)
		result = T_FAIL;
	else if ((T7_nfails == 0) && (T7_nprobs == 0))
		result = T_PASS;

	return(result);
}

static char	*a7 =	"A call to isc_task_create() creates a task that can "
			"receive events.";

void
t7(void) {
	int	result;

	t_assert("tasks", 7, T_REQUIRED, a7);
	result = t_tasks7();
	t_result(result);
}

#define	T10_SENDERCNT	3
#define	T10_TYPECNT	4
#define	T10_TAGCNT	5
#define	T10_NEVENTS	(T10_SENDERCNT*T10_TYPECNT*T10_TAGCNT)
#define	T_CONTROL	99999

static int		T10_nprobs;
static int		T10_nfails;
static int		T10_startflag;
static int		T10_shutdownflag;
static int		T10_eventcnt;
static isc_mutex_t	T10_mx;
static isc_condition_t	T10_cv;

static void		*T10_purge_sender;
static isc_eventtype_t	T10_purge_type_first;
static isc_eventtype_t	T10_purge_type_last;
static void		*T10_purge_tag;
static int		T10_testrange;

static void
t10_event1(isc_task_t *task, isc_event_t *event) {

	isc_result_t	isc_result;

	task = task;

	isc_result = isc_mutex_lock(&T10_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %s\n",
				isc_result_totext(isc_result));
		++T10_nprobs;
	}

	while (T10_startflag == 0) {
		isc_result = isc_condition_wait(&T10_cv, &T10_mx);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_mutex_lock failed %s\n",
					isc_result_totext(isc_result));
			++T10_nprobs;
		}
	}

	isc_result = isc_mutex_unlock(&T10_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_unlock failed %s\n",
				isc_result_totext(isc_result));
		++T10_nprobs;
	}

	isc_event_free(&event);
}

static void
t10_event2(isc_task_t *task, isc_event_t *event) {

	int	sender_match;
	int	type_match;
	int	tag_match;

	task = task;

	sender_match = 0;
	type_match = 0;
	tag_match = 0;

	if (T_debug) {
		t_info("Event %p,%d,%d,%s\n",
			event->sender,
			(int) event->type,
			event->tag,
			event->attributes & ISC_EVENTATTR_NOPURGE ? "NP" : "P");
	}

	if ((T10_purge_sender == 0) ||
	    (T10_purge_sender == event->sender)) {
		sender_match = 1;
	}
	if (T10_testrange == 0) {
		if (T10_purge_type_first == event->type) {
			type_match = 1;
		}
	}
	else {
		if ((T10_purge_type_first <= event->type) &&
		    (event->type <= T10_purge_type_last)) {
			type_match = 1;
		}
	}
	if ((T10_purge_tag == NULL) ||
	    (T10_purge_tag == event->tag)) {
		tag_match = 1;
	}
		
	if (sender_match && type_match && tag_match) {
		if (event->attributes & ISC_EVENTATTR_NOPURGE) {
			t_info("event %p,%d,%d matched but was not purgable\n",
				event->sender, (int) event->type, event->tag);
			++T10_eventcnt;
		}
		else {
			t_info("*** event %p,%d,%d not purged\n",
				event->sender, (int) event->type, event->tag);
		}
	}
	else {
		++T10_eventcnt;
	}
	isc_event_free(&event);
}


static void
t10_sde(isc_task_t *task, isc_event_t *event) {

	isc_result_t	isc_result;

	task = task;

	isc_result = isc_mutex_lock(&T10_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %s\n",
				isc_result_totext(isc_result));
		++T10_nprobs;
	}

	++T10_shutdownflag;

	isc_result = isc_condition_signal(&T10_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_condition_signal failed %s\n",
				isc_result_totext(isc_result));
		++T10_nprobs;
	}

	isc_result = isc_mutex_unlock(&T10_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_unlock failed %s\n",
				isc_result_totext(isc_result));
		++T10_nprobs;
	}

	isc_event_free(&event);
}

static void
t_taskpurge_x(	int sender, int type, int tag,
		int purge_sender, int purge_type_first, int purge_type_last, void *purge_tag,
		int exp_nevents, int *nfails, int *nprobs, int testrange) {

	char		*p;
	isc_mem_t	*mctx;
	isc_taskmgr_t	*tmgr;
	isc_task_t	*task;
	unsigned int	workers;
	isc_result_t	isc_result;
	isc_event_t	*event;
	isc_time_t	now;
	isc_interval_t	interval;
	int		sender_cnt;
	int		type_cnt;
	int		tag_cnt;
	int		event_cnt;
	int		cnt;
	int		nevents;
	isc_event_t	*eventtab[T10_NEVENTS];


	T10_startflag = 0;
	T10_shutdownflag = 0;
	T10_eventcnt = 0;
	T10_purge_sender = (void *) purge_sender;
	T10_purge_type_first = (isc_eventtype_t) purge_type_first;
	T10_purge_type_last = (isc_eventtype_t) purge_type_last;
	T10_purge_tag = purge_tag;
	T10_testrange = testrange;

	workers = 2;
	p = t_getenv("ISC_TASK_WORKERS");
	if (p != NULL)
		workers = atoi(p);

	mctx = NULL;
	isc_result = isc_mem_create(0, 0, &mctx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mem_create failed %s\n",
				isc_result_totext(isc_result));
		++*nprobs;
		return;
	}

	isc_result = isc_mutex_init(&T10_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_init failed %s\n",
				isc_result_totext(isc_result));
		isc_mem_destroy(&mctx);
		++*nprobs;
		return;
	}

	isc_result = isc_condition_init(&T10_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_condition_init failed %s\n",
				isc_result_totext(isc_result));
		isc_mem_destroy(&mctx);
		isc_mutex_destroy(&T10_mx);
		++*nprobs;
		return;
	}

	tmgr = NULL;
	isc_result = isc_taskmgr_create(mctx, workers, 0, &tmgr);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_taskmgr_create failed %s\n",
				isc_result_totext(isc_result));
		isc_mem_destroy(&mctx);
		isc_mutex_destroy(&T10_mx);
		isc_condition_destroy(&T10_cv);
		++*nprobs;
		return;
	}

	task = NULL;
	isc_result = isc_task_create(tmgr, mctx, 0, &task);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_create failed %s\n",
				isc_result_totext(isc_result));
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		isc_mutex_destroy(&T10_mx);
		isc_condition_destroy(&T10_cv);
		++*nprobs;
		return;
	}

	isc_result = isc_task_onshutdown(task, t10_sde, NULL);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_onshutdown returned %s\n",
				isc_result_totext(isc_result));
		isc_task_destroy(&task);
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		isc_mutex_destroy(&T10_mx);
		isc_condition_destroy(&T10_cv);
		++*nprobs;
		return;
	}

	/* block the task on T10_cv */
	event = isc_event_allocate(	mctx,
					(void *) 1,
					(isc_eventtype_t) T_CONTROL,
					t10_event1,
					NULL,
					sizeof(*event));

	isc_task_send(task, &event);

	/*
	 * fill the task's queue with some messages with varying
	 * sender, type, tag, and purgable attribute values
	 */

	event_cnt = 0;
	for (sender_cnt = 0; sender_cnt < T10_SENDERCNT; ++sender_cnt) {
		for (type_cnt = 0; type_cnt < T10_TYPECNT; ++type_cnt) {
			for (tag_cnt = 0; tag_cnt < T10_TAGCNT; ++tag_cnt) {
				eventtab[event_cnt] = isc_event_allocate(
							mctx,
							(void *) (sender + sender_cnt),
							(isc_eventtype_t) (type + type_cnt),
							t10_event2,
							NULL,
							sizeof(*event));
				
				eventtab[event_cnt]->tag = (void *)((int)tag + tag_cnt);

				/* make all odd message non-purgable */
				if ((sender_cnt % 2) && (type_cnt %2) && (tag_cnt %2))
					eventtab[event_cnt]->attributes |= ISC_EVENTATTR_NOPURGE;
				++event_cnt;
			}
		}
	}

	for (cnt = 0; cnt < event_cnt; ++cnt)
		isc_task_send(task, &eventtab[cnt]);

	if (T_debug)
		t_info("%d events queued\n", cnt);

	if (testrange == 0) {
		/* we're testing isc_task_purge */
		nevents = isc_task_purge(task,
					(void *) purge_sender,
					(isc_eventtype_t) purge_type_first,
					purge_tag);
		if (nevents != exp_nevents) {
			t_info("*** isc_task_purge returned %d, expected %d\n",
				nevents, exp_nevents);
			++*nfails;
		}
		else if (T_debug)
			t_info("isc_task_purge returned %d\n", nevents);
	}
	else {
		/* we're testing isc_task_purgerange */
		nevents = isc_task_purgerange(task,
					(void *) purge_sender,
					(isc_eventtype_t) purge_type_first,
					(isc_eventtype_t) purge_type_last,
					purge_tag);
		if (nevents != exp_nevents) {
			t_info("*** isc_task_purgerange returned %d, expected %d\n",
				nevents, exp_nevents);
			++*nfails;
		}
		else if (T_debug)
			t_info("isc_task_purgerange returned %d\n", nevents);
	}

	isc_result = isc_mutex_lock(&T10_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %s\n",
				isc_result_totext(isc_result));
		isc_task_destroy(&task);
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		isc_mutex_destroy(&T10_mx);
		isc_condition_destroy(&T10_cv);
		++*nprobs;
		return;
	}

	/* unblock the task, allowing event processing */
	T10_startflag = 1;
	isc_result = isc_condition_signal(&T10_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_condition_signal failed %s\n",
			isc_result_totext(isc_result));
		++*nprobs;
	}

	isc_task_shutdown(task);

	interval.seconds = 5;
	interval.nanoseconds = 0;

	/* wait for shutdown processing to complete */
	while (T10_shutdownflag == 0) {
		isc_result = isc_time_nowplusinterval(&now, &interval);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_time_nowplusinterval failed %s\n",
					isc_result_totext(isc_result));
			isc_task_detach(&task);
			isc_taskmgr_destroy(&tmgr);
			isc_mem_destroy(&mctx);
			isc_mutex_destroy(&T10_mx);
			isc_condition_destroy(&T10_cv);
			++*nprobs;
			return;
		}
	
		isc_result = isc_condition_waituntil(&T10_cv, &T10_mx, &now);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_condition_waituntil returned %s\n",
					isc_result_totext(isc_result));
			isc_task_detach(&task);
			isc_taskmgr_destroy(&tmgr);
			isc_mem_destroy(&mctx);
			isc_mutex_destroy(&T10_mx);
			isc_condition_destroy(&T10_cv);
			++*nfails;
			return;
		}
	}

	isc_result = isc_mutex_unlock(&T10_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_unlock failed %s\n",
				isc_result_totext(isc_result));
		++*nprobs;
	}

	isc_task_detach(&task);
	isc_taskmgr_destroy(&tmgr);
	isc_mem_destroy(&mctx);
	isc_mutex_destroy(&T10_mx);
	isc_condition_destroy(&T10_cv);

	if (T_debug)
		t_info("task processed %d events\n", T10_eventcnt);

	if ((T10_eventcnt + nevents) != event_cnt) {
		t_info("*** processed %d, purged %d, total %d\n",
			T10_eventcnt, nevents, event_cnt);
		++*nfails;
	}
}

static int
t_tasks10() {
	int	result;

	T10_nprobs = 0;
	T10_nfails = 0;

	/* try purging on a specific sender */
	t_info("testing purge on 2,4,8 expecting 1\n");
	t_taskpurge_x( 1, 4, 7, 2, 4, 4, (void *) 8, 1, &T10_nfails, &T10_nprobs, 0);

	/* try purging on all senders */
	t_info("testing purge on 0,4,8 expecting 3\n");
	t_taskpurge_x( 1, 4, 7, 0, 4, 4, (void *) 8, 3, &T10_nfails, &T10_nprobs, 0);

	/* try purging on all senders, specified type, all tags */
	t_info("testing purge on 0,4,0 expecting 15\n");
	t_taskpurge_x( 1, 4, 7, 0, 4, 4, NULL, 15, &T10_nfails, &T10_nprobs, 0);

	/* try purging on a specified tag, no such type */
	t_info("testing purge on 0,99,8 expecting 0\n");
	t_taskpurge_x( 1, 4, 7, 0, 99, 99, (void *) 8, 0, &T10_nfails, &T10_nprobs, 0);

	/* try purging on specified sender, type, all tags */
	t_info("testing purge on 0,5,0 expecting 5\n");
	t_taskpurge_x( 1, 4, 7, 3, 5, 5, NULL, 5, &T10_nfails, &T10_nprobs, 0);

	result = T_UNRESOLVED;

	if ((T10_nfails == 0) && (T10_nprobs == 0))
		result = T_PASS;
	else if (T10_nfails != 0)
		result = T_FAIL;

	return(result);
}

static char	*a10 =	"A call to isc_task_purge(task, sender, type, tag) purges "
			"all events of type 'type' and with tag 'tag' not marked as "
			"unpurgable from sender from the task's queue and returns "
			"the number of events purged.";

void
t10(void) {
	int	result;

	t_assert("tasks", 10, T_REQUIRED, a10);
	result = t_tasks10();
	t_result(result);
}

static int		T11_nprobs;
static int		T11_nfails;
static int		T11_startflag;
static int		T11_shutdownflag;
static int		T11_eventcnt;
static isc_mutex_t	T11_mx;
static isc_condition_t	T11_cv;


static void
t11_event1(isc_task_t *task, isc_event_t *event) {

	isc_result_t	isc_result;

	task = task;

	isc_result = isc_mutex_lock(&T11_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %s\n",
				isc_result_totext(isc_result));
		++T11_nprobs;
	}

	while (T11_startflag == 0) {
		isc_result = isc_condition_wait(&T11_cv, &T11_mx);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_mutex_lock failed %s\n",
					isc_result_totext(isc_result));
			++T11_nprobs;
		}
	}

	isc_result = isc_mutex_unlock(&T11_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_unlock failed %s\n",
				isc_result_totext(isc_result));
		++T11_nprobs;
	}

	isc_event_free(&event);
}

static void
t11_event2(isc_task_t *task, isc_event_t *event) {

	task = task;
	++T11_eventcnt;
	isc_event_free(&event);
}


static void
t11_sde(isc_task_t *task, isc_event_t *event) {

	isc_result_t	isc_result;

	task = task;

	isc_result = isc_mutex_lock(&T11_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %s\n",
				isc_result_totext(isc_result));
		++T11_nprobs;
	}

	++T11_shutdownflag;

	isc_result = isc_condition_signal(&T11_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_condition_signal failed %s\n",
				isc_result_totext(isc_result));
		++T11_nprobs;
	}

	isc_result = isc_mutex_unlock(&T11_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_unlock failed %s\n",
				isc_result_totext(isc_result));
		++T11_nprobs;
	}

	isc_event_free(&event);
}

static int
t_tasks11(int purgable) {

	char		*p;
	isc_mem_t	*mctx;
	isc_taskmgr_t	*tmgr;
	isc_task_t	*task;
	isc_boolean_t	rval;
	unsigned int	workers;
	isc_result_t	isc_result;
	isc_event_t	*event1;
	isc_event_t	*event2;
	isc_time_t	now;
	isc_interval_t	interval;
	int		result;


	T11_startflag = 0;
	T11_shutdownflag = 0;
	T11_eventcnt = 0;

	workers = 2;
	p = t_getenv("ISC_TASK_WORKERS");
	if (p != NULL)
		workers = atoi(p);

	mctx = NULL;
	isc_result = isc_mem_create(0, 0, &mctx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mem_create failed %s\n",
				isc_result_totext(isc_result));
		return(T_UNRESOLVED);
	}

	isc_result = isc_mutex_init(&T11_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_init failed %s\n",
				isc_result_totext(isc_result));
		isc_mem_destroy(&mctx);
		return(T_UNRESOLVED);
	}

	isc_result = isc_condition_init(&T11_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_condition_init failed %s\n",
				isc_result_totext(isc_result));
		isc_mem_destroy(&mctx);
		isc_mutex_destroy(&T11_mx);
		return(T_UNRESOLVED);
	}

	tmgr = NULL;
	isc_result = isc_taskmgr_create(mctx, workers, 0, &tmgr);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_taskmgr_create failed %s\n",
				isc_result_totext(isc_result));
		isc_mem_destroy(&mctx);
		isc_mutex_destroy(&T11_mx);
		isc_condition_destroy(&T11_cv);
		return(T_UNRESOLVED);
	}

	task = NULL;
	isc_result = isc_task_create(tmgr, mctx, 0, &task);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_create failed %s\n",
				isc_result_totext(isc_result));
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		isc_mutex_destroy(&T11_mx);
		isc_condition_destroy(&T11_cv);
		return(T_UNRESOLVED);
	}

	isc_result = isc_task_onshutdown(task, t11_sde, NULL);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_task_onshutdown returned %s\n",
				isc_result_totext(isc_result));
		isc_task_destroy(&task);
		isc_taskmgr_destroy(&tmgr);
		isc_mem_destroy(&mctx);
		isc_mutex_destroy(&T11_mx);
		isc_condition_destroy(&T11_cv);
		return(T_UNRESOLVED);
	}

	/* block the task on T11_cv */
	event1 = isc_event_allocate(	mctx,
					(void *) 1,
					(isc_eventtype_t) 1,
					t11_event1,
					NULL,
					sizeof(*event1));

	isc_task_send(task, &event1);

	event2 = isc_event_allocate(	mctx,
					(void *) 1,
					(isc_eventtype_t) 1,
					t11_event2,
					NULL,
					sizeof(*event2));
				
	if (purgable)
		event2->attributes &= ~ISC_EVENTATTR_NOPURGE;
	else
		event2->attributes |= ISC_EVENTATTR_NOPURGE;

	isc_task_send(task, &event2);

	rval = isc_task_purgeevent(task, event2);
	if (rval != (purgable ? ISC_TRUE : ISC_FALSE)) {
		t_info("isc_task_purgeevent returned %s, expected %s\n",
			(rval ? "ISC_TRUE" : "ISC_FALSE"),
			(purgable ? "ISC_TRUE" : "ISC_FALSE"));
		++T11_nfails;
	}

	isc_result = isc_mutex_lock(&T11_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_lock failed %s\n",
				isc_result_totext(isc_result));
		++T11_nprobs;
	}

	/* unblock the task, allowing event processing */
	T11_startflag = 1;
	isc_result = isc_condition_signal(&T11_cv);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_condition_signal failed %s\n",
				isc_result_totext(isc_result));
		++T11_nprobs;
	}

	isc_task_shutdown(task);

	interval.seconds = 5;
	interval.nanoseconds = 0;

	/* wait for shutdown processing to complete */
	while (T11_shutdownflag == 0) {
		isc_result = isc_time_nowplusinterval(&now, &interval);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_time_nowplusinterval failed %s\n",
					isc_result_totext(isc_result));
			++T11_nprobs;
		}
	
		isc_result = isc_condition_waituntil(&T11_cv, &T11_mx, &now);
		if (isc_result != ISC_R_SUCCESS) {
			t_info("isc_condition_waituntil returned %s\n",
					isc_result_totext(isc_result));
			++T11_nprobs;
		}
	}

	isc_result = isc_mutex_unlock(&T11_mx);
	if (isc_result != ISC_R_SUCCESS) {
		t_info("isc_mutex_unlock failed %s\n",
				isc_result_totext(isc_result));
		++T11_nprobs;
	}

	isc_task_detach(&task);
	isc_taskmgr_destroy(&tmgr);
	isc_mem_destroy(&mctx);
	isc_mutex_destroy(&T11_mx);
	isc_condition_destroy(&T11_cv);

	if (T11_eventcnt != (purgable ? 0 : 1)) {
		t_info("Event was %s purged\n", (purgable ? "not" : "unexpectedly"));
		++T11_nfails;
	}

	result = T_UNRESOLVED;

	if ((T11_nfails == 0) && (T11_nprobs == 0))
		result = T_PASS;
	else if (T11_nfails)
		result = T_FAIL;

	return(result);
}

static char	*a11 =	"When the event is marked as purgable, a call to "
			"isc_task_purgeevent(task, event) purges the event 'event' "
			"from the task's queue and returns ISC_TRUE.";
			
void
t11(void) {
	int	result;
	t_assert("tasks", 11, T_REQUIRED, a11);
	result = t_tasks11(1);
	t_result(result);
}

static char	*a12 =	"When the event is not marked as purgable, a call to "
			"isc_task_purgeevent(task, event) does not purge the event 'event' "
			"from the task's queue and returns ISC_FALSE.";

static int
t_tasks12() {
	return(t_tasks11(0));
}

void
t12(void) {
	int	result;
	t_assert("tasks", 12, T_REQUIRED, a12);
	result = t_tasks12();
	t_result(result);
}

static int	T13_nfails;
static int	T13_nprobs;

static char	*a13 =	"A call to isc_event_purgerange(task, sender, first, last, tag) "
			"purges all events not marked unpurgable from sender 'sender' and "
			"of type within the range 'first' to 'last' inclusive from "
			"the task's event queue and returns the number of tasks purged.";
			
static int
t_tasks13() {
	int	result;

	T13_nfails = 0;
	T13_nprobs = 0;

	/*
	 * first lets try the same cases we used in t10
	 */

	/* try purging on a specific sender */
	t_info("testing purge on 2,4,8 expecting 1\n");
	t_taskpurge_x( 1, 4, 7, 2, 4, 4, (void *) 8, 1, &T13_nfails, &T13_nprobs, 1);

	/* try purging on all senders */
	t_info("testing purge on 0,4,8 expecting 3\n");
	t_taskpurge_x( 1, 4, 7, 0, 4, 4, (void *) 8, 3, &T13_nfails, &T13_nprobs, 1);

	/* try purging on all senders, specified type, all tags */
	t_info("testing purge on 0,4,0 expecting 15\n");
	t_taskpurge_x( 1, 4, 7, 0, 4, 4, NULL, 15, &T13_nfails, &T13_nprobs, 1);

	/* try purging on a specified tag, no such type */
	t_info("testing purge on 0,99,8 expecting 0\n");
	t_taskpurge_x( 1, 4, 7, 0, 99, 99, (void *) 8, 0, &T13_nfails, &T13_nprobs, 1);

	/* try purging on specified sender, type, all tags */
	t_info("testing purge on 3,5,0 expecting 5\n");
	t_taskpurge_x( 1, 4, 7, 3, 5, 5, 0, 5, &T13_nfails, &T13_nprobs, 1);

	/*
	 * now lets try some ranges
	 */

	t_info("testing purgerange on 2,4-5,8 expecting 2\n");
	t_taskpurge_x( 1, 4, 7, 2, 4, 5, (void *) 8, 1, &T13_nfails, &T13_nprobs, 1);

	/* try purging on all senders */
	t_info("testing purge on 0,4-5,8 expecting 5\n");
	t_taskpurge_x( 1, 4, 7, 0, 4, 5, (void *) 8, 5, &T13_nfails, &T13_nprobs, 1);

	/* try purging on all senders, specified type, all tags */
	t_info("testing purge on 0,5-6,0 expecting 28\n");
	t_taskpurge_x( 1, 4, 7, 0, 5, 6, NULL, 28, &T13_nfails, &T13_nprobs, 1);

	/* try purging on a specified tag, no such type */
	t_info("testing purge on 0,99-101,8 expecting 0\n");
	t_taskpurge_x( 1, 4, 7, 0, 99, 101, (void *) 8, 0, &T13_nfails, &T13_nprobs, 1);

	/* try purging on specified sender, type, all tags */
	t_info("testing purge on 3,5-6,0 expecting 10\n");
	t_taskpurge_x( 1, 4, 7, 3, 5, 6, NULL, 10, &T13_nfails, &T13_nprobs, 1);


	result = T_UNRESOLVED;

	if ((T13_nfails == 0) && (T13_nprobs == 0))
		result = T_PASS;
	else if (T13_nfails)
		result = T_FAIL;

	return(result);
}


void
t13(void) {
	int	result;

	t_assert("tasks", 13, T_REQUIRED, a13);
	result = t_tasks13();
	t_result(result);
}

testspec_t	T_testlist[] = {
	{	t1,	"basic task subsystem"	},
	{	t2,	"maxtasks"		},
	{	t3,	"isc_task_shutdown"	},
	{	t4,	"isc_task_shutdown"	},
	{	t7,	"isc_task_create"	},
	{	t10,	"isc_task_purge"	},
	{	t11,	"isc_task_purgeevent"	},
	{	t12,	"isc_task_purgeevent"	},
	{	t13,	"isc_task_purgerange"	},
	{	NULL,	NULL			}
};
