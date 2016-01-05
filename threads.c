/* threads.c - C1X threads draft implementation as per
 *             WG14 N1425 Draft 2009-11-24 ISO/IEC 9899:201x
 *             http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1425.pdf
 *             Using POSIX threads as backend
 *
 * Copyright (c) 2010, Michael Gruhn <michael-gruhn@web.de>
 * Some rights reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* XXX: FIXME: pthreads return a void* but C1X requires a return of int, we
 * currently just use the void* to store the int in the pointer value, while
 * this works it might pose potential problems
 */
/* XXX: tss_*() functions are as in POSIX threads, that is 'key' must be synced
 * by user. Hopefully C1X changes it so it is save to use it multithreaded,
 * though after reading countless documents about it, it seems they haven't
 * tightly defined the whole behaviour yet. However since it is closely designed
 * after POSIX threads the author goes with that behaviour.
 */
/* XXX: the pthread_mutex_timedlock function is optional, for a crude hacked
 * version without the use of it refere to the last revision of this file
 */

#include <errno.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "threads.h"


void call_once(once_flag *flag, void(*func)(void))
{
	pthread_once(flag,func);
}


int cnd_broadcast(cnd_t *cond)
{
	return pthread_cond_broadcast(cond)==0?thrd_success:thrd_error;
}


void cnd_destroy(cnd_t *cond)
{
	pthread_cond_destroy(cond);
}


int cnd_init(cnd_t *cond)
{
	return pthread_cond_init(cond,NULL)==0?thrd_success:thrd_error;
}


int cnd_signal(cnd_t *cond)
{
	return pthread_cond_signal(cond)==0?thrd_success:thrd_error;
}


int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const xtime *xt)
{
	struct timespec ts;
	ts.tv_sec = xt->sec;
	ts.tv_nsec = xt->nsec;
	switch( pthread_cond_timedwait(cond,mtx,&ts) )
	{
		case 0:
			return thrd_success;
		case ETIMEDOUT:
			return thrd_timeout;
		default:
			return thrd_error;
	}
}

int cnd_wait(cnd_t *cond, mtx_t *mtx)
{
	return pthread_cond_wait(cond,mtx)==0?thrd_success:thrd_error;
}




/* XXX: FIXME: pthreads return a void* but C1X requires a return of int, we
 * currently just use the void* to store the int in the pointer value, while
 * this works it might pose potential problems
 */
int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
	return pthread_create(thr,NULL,(void*(*)(void*))func,arg)==0?
		thrd_success:thrd_error;
}


thrd_t thrd_current(void)
{
	return pthread_self();
}


int thrd_detach(thrd_t thr)
{
	return pthread_detach(thr)==0?thrd_success:thrd_error;
}


int thrd_equal(thrd_t thr0, thrd_t thr1)
{
	return pthread_equal(thr0,thr1);
}


/* XXX: FIXME: see 'thrd_create()' above
 */
void thrd_exit(int res)
{
	pthread_exit((void*)res);
}


/* XXX: FIXME: see 'thrd_create()' above
 */
int thrd_join(thrd_t thr, int *res)
{
	int ret;
	void *r;
	ret=pthread_join(thr, &r);
	*res=(int)(r);
	return ret==0?thrd_success:thrd_error;
}


void thrd_sleep(const xtime *xt)
{
	struct timespec t;
	t.tv_sec = xt->sec;
	t.tv_nsec = xt->nsec;
	nanosleep( &t , NULL );
}


void thrd_yield(void)
{
	sched_yield();
}




void mtx_destroy(mtx_t *mtx)
{
	while( pthread_mutex_destroy(mtx) == EBUSY );
}


int mtx_init(mtx_t *mtx, int type)
{
	int i = 0;
	pthread_mutexattr_t a;
	pthread_mutexattr_init(&a);
	if(type&mtx_recursive)
	{
		pthread_mutexattr_settype(&a,PTHREAD_MUTEX_RECURSIVE);
	}
	if( type&mtx_plain )
	{
		i++;
	}
	if( type&mtx_try )
	{
		i++;
	}
	if( type&mtx_timed )
	{
		i++;
	}
	if( i != 1 )
	{
		return thrd_error;
	}
	return pthread_mutex_init(mtx,&a)==0?thrd_success:thrd_error;
}


int mtx_lock(mtx_t *mtx)
{
	return pthread_mutex_lock(mtx)==0?thrd_success:thrd_error;
}


int mtx_timedlock(mtx_t *mtx, const xtime *xt)
{
	struct timespec tp;
	tp.tv_sec=xt->sec;
	tp.tv_nsec=xt->nsec;
	
	/* XXX: the pthread_mutex_timedlock function is optional, for a crude hacked
	 * version without the use of it refer to the last revision of this file
	 */
	switch( pthread_mutex_timedlock(mtx,&tp) )
	{
		case 0:
			return thrd_success;
		case ETIMEDOUT:
			return thrd_timeout;
		default:
			return thrd_error;
	}
}


int mtx_trylock(mtx_t *mtx)
{
	switch( pthread_mutex_trylock(mtx) )
	{
		case 0:
			return thrd_success;
		case EBUSY:
			return thrd_busy;
		default:
			return thrd_error;
	}
}


int mtx_unlock(mtx_t *mtx)
{
	return pthread_mutex_unlock(mtx)==0?thrd_success:thrd_error;
}




int tss_create(tss_t *key, tss_dtor_t dtor)
{
	return pthread_key_create(key, dtor)==0?thrd_success:thrd_error;
}


void tss_delete(tss_t key)
{
	pthread_key_delete(key);
}


void *tss_get(tss_t key)
{
	return pthread_getspecific(key);
}


int tss_set(tss_t key, void *val)
{
	return pthread_setspecific(key, val)==0?thrd_success:thrd_error;
}


int xtime_get(xtime *xt, int base)
{
	struct timespec ts;
	if( base != TIME_UTC || clock_gettime(CLOCK_REALTIME, &ts) == -1 )
	{
		return 0;
	}
	xt->sec = ts.tv_sec;
	xt->nsec = ts.tv_nsec;
	return base;
}


