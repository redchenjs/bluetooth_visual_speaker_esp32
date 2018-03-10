/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GOS_QT_H
#define _GOS_QT_H

#if GFX_USE_OS_QT

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define DECLARE_THREAD_FUNCTION(fnName, param)	threadreturn_t fnName(void *param)
#define DECLARE_THREAD_STACK(name, sz)          uint8_t name[0]
#define THREAD_RETURN(retval)					return retval

#define TIME_IMMEDIATE				0
#define TIME_INFINITE				((delaytime_t)-1)
#define MAX_SEMAPHORE_COUNT			((semcount_t)(((unsigned long)((semcount_t)(-1))) >> 1))
#define LOW_PRIORITY				2
#define NORMAL_PRIORITY				3
#define HIGH_PRIORITY				4

typedef bool bool_t;
typedef int systemticks_t;
typedef int delaytime_t;
typedef void* gfxMutex;
typedef void* gfxSem;
typedef int semcount_t;
typedef int threadreturn_t;
typedef int threadpriority_t;
typedef void* gfxThreadHandle;

#ifdef __cplusplus
extern "C" {
#endif

void _gosInit();
void _gosDeinit();

void gfxHalt(const char* msg);
void gfxExit(void);
void* gfxAlloc(size_t sz);
void gfxFree(void* ptr);
void gfxYield(void);
void gfxSleepMilliseconds(delaytime_t ms);
void gfxSleepMicroseconds(delaytime_t us);
systemticks_t gfxSystemTicks(void);
systemticks_t gfxMillisecondsToTicks(delaytime_t ms);
void gfxSystemLock(void);
void gfxSystemUnlock(void);
void gfxMutexInit(gfxMutex *pmutex);
void gfxMutexDestroy(gfxMutex *pmutex);
void gfxMutexEnter(gfxMutex *pmutex);
void gfxMutexExit(gfxMutex *pmutex);
void gfxSemInit(gfxSem *psem, semcount_t val, semcount_t limit);
void gfxSemDestroy(gfxSem *psem);
bool_t gfxSemWait(gfxSem *psem, delaytime_t ms);
bool_t gfxSemWaitI(gfxSem *psem);
void gfxSemSignal(gfxSem *psem);
void gfxSemSignalI(gfxSem *psem);
gfxThreadHandle gfxThreadCreate(void *stackarea, size_t stacksz, threadpriority_t prio, DECLARE_THREAD_FUNCTION((*fn),p), void *param);
threadreturn_t gfxThreadWait(gfxThreadHandle thread);
gfxThreadHandle gfxThreadMe(void);
void gfxThreadClose(gfxThreadHandle thread);

#ifdef __cplusplus
}
#endif

#endif /* GFX_USE_OS_QT */
#endif /* _GOS_QT_H */
