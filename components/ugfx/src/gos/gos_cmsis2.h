/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

/**
 * @file    src/gos/gos_cmsis2.h
 * @brief   GOS - Operating System Support header file for CMSIS 2.0 RTOS.
 */

#ifndef _GOS_CMSIS2_H
#define _GOS_CMSIS2_H

#if GFX_USE_OS_CMSIS2

#include <stdbool.h>
#include "cmsis_os2.h"

#ifndef GFX_OS_HEAP_SIZE
	#define GFX_OS_HEAP_SIZE 10240
#endif

/*===========================================================================*/
/* Type definitions                                                          */
/*===========================================================================*/

typedef bool				bool_t;

#define TIME_IMMEDIATE		0
#define TIME_INFINITE		osWaitForever
typedef uint32_t			delaytime_t;
typedef uint32_t			systemticks_t;
typedef uint16_t			semcount_t;
typedef void				threadreturn_t;
typedef osPriority_t		threadpriority_t;

#define MAX_SEMAPHORE_COUNT	65535UL
#define LOW_PRIORITY		osPriorityLow
#define NORMAL_PRIORITY		osPriorityNormal
#define HIGH_PRIORITY		osPriorityHigh

typedef osSemaphoreId_t		gfxSem;

typedef osMutexId_t 		gfxMutex;

typedef osThreadId_t		gfxThreadHandle;

#define DECLARE_THREAD_STACK(name, sz)			uint8_t name[1];	// Some compilers don't allow zero sized arrays. Let's waste one byte
#define DECLARE_THREAD_FUNCTION(fnName, param)	threadreturn_t fnName(void* param)
#define THREAD_RETURN(retval)

/*===========================================================================*/
/* Function declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#define gfxExit()					os_error(0)
#define gfxHalt(msg)				os_error(1)
#define gfxSystemTicks()			osKernelGetSysTimerCount()
#define gfxMillisecondsToTicks(ms)	(1000*(ms)/osKernelGetTickFreq())
#define gfxSystemLock()				osKernelLock()
#define gfxSystemUnlock()			osKernelUnlock()
#define gfxSleepMilliseconds(ms) 	osDelay(ms)

void gfxMutexInit(gfxMutex* pmutex);
#define gfxMutexDestroy(pmutex)		osMutexDelete(*(pmutex))
#define gfxMutexEnter(pmutex)		osMutexAcquire(*(pmutex), TIME_INFINITE)
#define gfxMutexExit(pmutex)		osMutexRelease(*(pmutex))

void gfxSemInit(gfxSem* psem, semcount_t val, semcount_t limit);
#define gfxSemDestroy(psem)		osSemaphoreDelete(*(psem))
bool_t gfxSemWait(gfxSem* psem, delaytime_t ms);
#define gfxSemWaitI(psem)		gfxSemWait((psem), 0)
#define gfxSemSignal(psem)		osSemaphoreRelease(*(psem))
#define gfxSemSignalI(psem)		osSemaphoreRelease(*(psem))

gfxThreadHandle gfxThreadCreate(void* stackarea, size_t stacksz, threadpriority_t prio, DECLARE_THREAD_FUNCTION((*fn),p), void* param);
#define gfxYield()					osThreadYield()
#define gfxThreadMe()				osThreadGetId()
#define gfxThreadClose(thread)		{}

#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Use the generic heap handling                                             */
/*===========================================================================*/

#define GOS_NEED_X_HEAP TRUE
#include "gos_x_heap.h"

#endif /* GFX_USE_OS_CMSIS */
#endif /* _GOS_CMSIS_H */
