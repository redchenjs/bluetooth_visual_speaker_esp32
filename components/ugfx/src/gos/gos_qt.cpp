/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "../../gfx.h"

#if GFX_USE_OS_QT
#include <QMutex>
#include <QSemaphore>
#include <QThread>
#include <QElapsedTimer>


class Thread : public QThread
{
public:
    typedef threadreturn_t (*fptr)(void* param);

    void setFunction(fptr function, void* param)
    {
        _function = function;
        _param = param;
    }

    threadreturn_t returnValue()
    {
        return _returnValue;
    }

    virtual void run() override
    {
        if (!_function) {
            return;
        }

        _returnValue = _function(_param);
    }

private:
    fptr _function;
    void* _param;
    threadreturn_t _returnValue;
};

static QElapsedTimer _systickTimer;
static QMutex _systemMutex;

void _gosInit(void)
{
    _systickTimer.start();
}

void _gosPostInit(void)
{
}

void _gosDeinit(void)
{
}

void gfxHalt(const char *msg)
{
    volatile uint32_t dummy;

    (void)msg;

    while(1) {
        dummy++;
    }
}

void gfxExit(void)
{
    volatile uint32_t dummy;

    while(1) {
        dummy++;
    }
}

void* gfxAlloc(size_t sz)
{
    return malloc(sz);
}

void gfxFree(void* ptr)
{
    free(ptr);
}

void gfxYield(void)
{
    QThread::msleep(0);
}

void gfxSleepMilliseconds(delaytime_t ms)
{
    QThread::msleep(ms);
}

void gfxSleepMicroseconds(delaytime_t us)
{
    QThread::usleep(us);
}

systemticks_t gfxSystemTicks(void)
{
    return _systickTimer.elapsed();
}

systemticks_t gfxMillisecondsToTicks(delaytime_t ms)
{
    return ms;
}

void gfxSystemLock(void)
{
    _systemMutex.lock();
}

void gfxSystemUnlock(void)
{
    _systemMutex.unlock();
}

void gfxMutexInit(gfxMutex *pmutex)
{
    *pmutex = new QMutex;
}

void gfxMutexDestroy(gfxMutex *pmutex)
{
    delete static_cast<QMutex*>(*pmutex);
}

void gfxMutexEnter(gfxMutex *pmutex)
{
    static_cast<QMutex*>(*pmutex)->lock();
}

void gfxMutexExit(gfxMutex *pmutex)
{
    static_cast<QMutex*>(*pmutex)->unlock();
}

void gfxSemInit(gfxSem *psem, semcount_t val, semcount_t limit)
{
    *psem = new QSemaphore(limit);

    static_cast<QSemaphore*>(*psem)->release(val);
}

void gfxSemDestroy(gfxSem *psem)
{
    delete static_cast<QSemaphore*>(*psem);
}

bool_t gfxSemWait(gfxSem *psem, delaytime_t ms)
{
    return static_cast<QSemaphore*>(*psem)->tryAcquire(1, ms);
}

bool_t gfxSemWaitI(gfxSem *psem)
{
    return static_cast<QSemaphore*>(*psem)->tryAcquire(1);
}

void gfxSemSignal(gfxSem *psem)
{
    static_cast<QSemaphore*>(*psem)->release(1);
}

void gfxSemSignalI(gfxSem *psem)
{
    static_cast<QSemaphore*>(*psem)->release(1);
}

gfxThreadHandle gfxThreadCreate(void *stackarea, size_t stacksz, threadpriority_t prio, DECLARE_THREAD_FUNCTION((*fn),p), void *param)
{
    Q_UNUSED(stackarea)

    Thread* thread = new Thread;
    thread->setFunction(fn, param);
    if (stacksz > 0) {
        thread->setStackSize(stacksz);
    }
    thread->start(static_cast<QThread::Priority>(prio));

    return static_cast<gfxThreadHandle>(thread);
}

threadreturn_t gfxThreadWait(gfxThreadHandle thread)
{
    Thread* t = static_cast<Thread*>(thread);

    threadreturn_t returnValue = t->returnValue();
    t->wait();
    t->exit();

    return returnValue;
}

gfxThreadHandle gfxThreadMe(void)
{
    return static_cast<Thread*>(QThread::currentThread());
}

void gfxThreadClose(gfxThreadHandle thread)
{
    static_cast<Thread*>(thread)->exit();
}

#endif /* GFX_USE_OS_QT */
