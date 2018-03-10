/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GOS_ZEPHYR_H
#define _GOS_ZEPHYR_H

#if GFX_USE_OS_ZEPHYR

// #include <stdbool.h>
// #include <stdint.h>

#include <zephyr.h>

	/*===========================================================================*/
	/* Type definitions                                                          */
	/*===========================================================================*/

	typedef bool	bool_t;
	typedef s8_t	int8_t;
	typedef u8_t	uint8_t;
	typedef s16_t	int16_t;
	typedef u16_t	uint16_t;
	typedef s32_t	int32_t;
	typedef u32_t	uint32_t;

	// typedef unsigned long	size_t;
	typedef s32_t 	delaytime_t;
	typedef u32_t	systemticks_t;
	typedef u32_t	semcount_t;
	typedef void	threadreturn_t;
	typedef int	threadpriority_t;

	#define DECLARE_THREAD_FUNCTION(fnName, param)\
		threadreturn_t fnName(void* param, void* p2, void* p3)

	#define DECLARE_THREAD_STACK(name, sz)\
		K_THREAD_STACK_DEFINE(name, sz)

	#define THREAD_RETURN(retval)		return

	// #define FALSE			0
	// #define TRUE				1
	#define TIME_IMMEDIATE			K_NO_WAIT
	#define TIME_INFINITE			K_FOREVER
	#define MAX_SEMAPHORE_COUNT		((semcount_t)(((unsigned long)((semcount_t)(-1))) >> 1))
	#define LOW_PRIORITY			CONFIG_NUM_PREEMPT_PRIORITIES-1
	#define NORMAL_PRIORITY			1
	#define HIGH_PRIORITY			0

	typedef struct k_sem gfxSem;

	typedef struct k_mutex gfxMutex;

	typedef k_tid_t gfxThreadHandle;

	/*===========================================================================*/
	/* Function declarations.                                                    */
	/*===========================================================================*/

	#ifdef __cplusplus
	extern "C" {
	#endif

	#define gfxHalt(msg)	do{}while(0)
	#define gfxExit()		do{}while(0)

	// Don't forget to set CONFIG_HEAP_MEM_POOL_SIZE
	 #define gfxAlloc(sz)					k_malloc(sz)
	 #define gfxFree(ptr)					k_free(ptr)
	 #define gfxRealloc(ptr, oldsz, newsz)	do{}while(0)

	 #define gfxYield()						k_yield()
	 #define gfxSleepMilliseconds(ms)		k_sleep(ms)
	 #define gfxSleepMicroseconds(us)		do{}while(0)
	 #define gfxMillisecondsToTicks(ms)		CONFIG_SYS_CLOCK_TICKS_PER_SEC*ms/1000
	 systemticks_t gfxSystemTicks();

	 #define gfxSystemLock()		k_sched_lock()
	 #define gfxSystemUnlock()		k_sched_unlock()

	 #define gfxMutexInit(pmutex)		k_mutex_init(pmutex)
	 #define gfxMutexDestroy(pmutex)	do{}while(0)
	 #define gfxMutexEnter(pmutex)		k_mutex_lock(pmutex, K_FOREVER)
	 #define gfxMutexExit(pmutex)		k_mutex_unlock(pmutex)

	 #define gfxSemInit(psem, val, limit) 	k_sem_init(psem, val, limit)
	 #define gfxSemDestroy(psem)			do{}while(0)
	 #define gfxSemWait(psem, ms)  			(k_sem_take(psem, ms) == 0) ? TRUE : FALSE
	 #define gfxSemWaitI(psem) 				(k_sem_take(psem, K_NO_WAIT) == 0) ? TRUE : FALSE
	 #define gfxSemSignal(psem)				k_sem_give(psem)
	 #define gfxSemSignalI(psem)			k_sem_give(psem)
	 #define gfxSemCounter(psem)			k_sem_count_get(psem)
	 #define gfxSemCounterI(psem)			k_sem_count_get(psem)

	#define gfxThreadCreate(stackarea, stacksz, prio, fn, param)\
		k_thread_spawn(stackarea, stacksz, fn, param, NULL, NULL, prio, 0, K_NO_WAIT)
	#define gfxThreadWait(thread)		0
	#define gfxThreadMe()				k_current_get()
	#define gfxThreadClose(thread)		k_thread_abort(thread)

	#ifdef __cplusplus
	}
	#endif

#endif /* GFX_USE_OS_ZEPHYR */
#endif /* _GOS_H */
/** @} */
