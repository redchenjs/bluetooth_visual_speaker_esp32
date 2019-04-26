/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "../../gfx.h"

#if GFX_USE_GQUEUE

#if GQUEUE_NEED_BUFFERS
	static gfxQueueGSync	bufferFreeList;
#endif

void _gqueueInit(void)
{
	#if GQUEUE_NEED_BUFFERS
		gfxQueueGSyncInit(&bufferFreeList);
	#endif
}

void _gqueueDeinit(void)
{
}

#if GQUEUE_NEED_ASYNC
	void gfxQueueASyncInit(gfxQueueASync *pqueue) {
		pqueue->head = pqueue->tail = 0;
	}

	gfxQueueASyncItem *gfxQueueASyncGet(gfxQueueASync *pqueue) {
		gfxQueueASyncItem	*pi;

		// This is just a shortcut to speed execution
		if (!pqueue->head)
			return 0;

		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		pi = gfxQueueASyncGetI(pqueue);
		gfxSystemUnlock(&mux);

		return pi;
	}
	gfxQueueASyncItem *gfxQueueASyncGetI(gfxQueueASync *pqueue) {
		gfxQueueASyncItem	*pi;

		if ((pi = pqueue->head)) {
			pqueue->head = pi->next;
			pi->next = 0;
		}

		return pi;
	}

	void gfxQueueASyncPut(gfxQueueASync *pqueue, gfxQueueASyncItem *pitem) {
		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		gfxQueueASyncPutI(pqueue, pitem);
		gfxSystemUnlock(&mux);
	}
	void gfxQueueASyncPutI(gfxQueueASync *pqueue, gfxQueueASyncItem *pitem) {
		if (!pitem) return;				// Safety
		pitem->next = 0;
		if (!pqueue->head) {
			pqueue->head = pqueue->tail = pitem;
		} else {
			pqueue->tail->next = pitem;
			pqueue->tail = pitem;
		}
	}

	void gfxQueueASyncPush(gfxQueueASync *pqueue, gfxQueueASyncItem *pitem) {
		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		gfxQueueASyncPushI(pqueue, pitem);
		gfxSystemUnlock(&mux);
	}
	void gfxQueueASyncPushI(gfxQueueASync *pqueue, gfxQueueASyncItem *pitem) {
		if (!pitem) return;				// Safety
		pitem->next = pqueue->head;
		pqueue->head = pitem;
		if (!pitem->next)
			pqueue->tail = pitem;
	}

	void gfxQueueASyncInsert(gfxQueueASync *pqueue, gfxQueueASyncItem *pitem, gfxQueueASyncItem *pafter) {
		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		gfxQueueASyncInsertI(pqueue, pitem, pafter);
		gfxSystemUnlock(&mux);
	}
	void gfxQueueASyncInsertI(gfxQueueASync *pqueue, gfxQueueASyncItem *pitem, gfxQueueASyncItem *pafter) {
		if (!pitem) return;				// Safety

		if (pafter && gfxQueueASyncIsInI(pqueue, pafter)) {
			pitem->next = pafter->next;
			pafter->next = pitem;
			if (pqueue->tail == pafter)
				pqueue->tail = pitem;
		} else {
			pitem->next = 0;
			if (!pqueue->head) {
				pqueue->head = pqueue->tail = pitem;
			} else {
				pqueue->tail->next = pitem;
				pqueue->tail = pitem;
			}
		}
	}

	void gfxQueueASyncRemove(gfxQueueASync *pqueue, gfxQueueASyncItem *pitem) {
		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		gfxQueueASyncRemoveI(pqueue, pitem);
		gfxSystemUnlock(&mux);
	}
	void gfxQueueASyncRemoveI(gfxQueueASync *pqueue, gfxQueueASyncItem *pitem) {
		gfxQueueASyncItem *pi;

		if (!pitem) return;				// Safety
		if (pqueue->head) {
			if (pqueue->head == pitem) {
				pqueue->head = pitem->next;
				pitem->next = 0;
			} else {
				for(pi = pqueue->head; pi->next; pi = pi->next) {
					if (pi->next == pitem) {
						pi->next = pitem->next;
						if (pqueue->tail == pitem)
							pqueue->tail = pi;
						pitem->next = 0;
						break;
					}
				}
			}
		}
	}

	bool_t gfxQueueASyncIsIn(gfxQueueASync *pqueue, const gfxQueueASyncItem *pitem) {
		bool_t	res;

		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		res = gfxQueueASyncIsInI(pqueue, pitem);
		gfxSystemUnlock(&mux);

		return res;
	}
	bool_t gfxQueueASyncIsInI(gfxQueueASync *pqueue, const gfxQueueASyncItem *pitem) {
		gfxQueueASyncItem *pi;

		for(pi = pqueue->head; pi; pi = pi->next) {
			if (pi == pitem)
				return TRUE;
		}
		return FALSE;
	}
#endif

#if GQUEUE_NEED_GSYNC
	void gfxQueueGSyncInit(gfxQueueGSync *pqueue) {
		pqueue->head = pqueue->tail = 0;
		gfxSemInit(&pqueue->sem, 0, MAX_SEMAPHORE_COUNT);
	}
	void gfxQueueGSyncDeinit(gfxQueueGSync *pqueue) {
		pqueue->head = pqueue->tail = 0;
		gfxSemDestroy(&pqueue->sem);
	}

	gfxQueueGSyncItem *gfxQueueGSyncGet(gfxQueueGSync *pqueue, delaytime_t ms) {
		gfxQueueGSyncItem	*pi;

		if (!gfxSemWait(&pqueue->sem, ms))
			return 0;

		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		pi = pqueue->head;
		pqueue->head = pi->next;
		pi->next = 0;
		gfxSystemUnlock(&mux);

		return pi;
	}
	gfxQueueGSyncItem *gfxQueueGSyncGetI(gfxQueueGSync *pqueue) {
		gfxQueueGSyncItem	*pi;

		if (!gfxSemWaitI(&pqueue->sem))
			return 0;

		pi = pqueue->head;
		pqueue->head = pi->next;
		pi->next = 0;
		return pi;
	}

	void gfxQueueGSyncPut(gfxQueueGSync *pqueue, gfxQueueGSyncItem *pitem) {
		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		gfxQueueGSyncPutI(pqueue, pitem);
		gfxSystemUnlock(&mux);
	}
	void gfxQueueGSyncPutI(gfxQueueGSync *pqueue, gfxQueueGSyncItem *pitem) {
		if (!pitem) return;				// Safety
		pitem->next = 0;
		if (!pqueue->head) {
			pqueue->head = pqueue->tail = pitem;
		} else {
			pqueue->tail->next = pitem;
			pqueue->tail = pitem;
		}
		gfxSemSignalI(&pqueue->sem);
	}

	void gfxQueueGSyncPush(gfxQueueGSync *pqueue, gfxQueueGSyncItem *pitem) {
		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		gfxQueueGSyncPushI(pqueue, pitem);
		gfxSystemUnlock(&mux);
	}
	void gfxQueueGSyncPushI(gfxQueueGSync *pqueue, gfxQueueGSyncItem *pitem) {
		if (!pitem) return;				// Safety
		pitem->next = pqueue->head;
		pqueue->head = pitem;
		if (!pitem->next)
			pqueue->tail = pitem;
		gfxSemSignalI(&pqueue->sem);
	}

	void gfxQueueGSyncInsert(gfxQueueGSync *pqueue, gfxQueueGSyncItem *pitem, gfxQueueASyncItem *pafter) {
		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		gfxQueueGSyncInsertI(pqueue, pitem, pafter);
		gfxSystemUnlock(&mux);
	}
	void gfxQueueGSyncInsertI(gfxQueueGSync *pqueue, gfxQueueGSyncItem *pitem, gfxQueueASyncItem *pafter) {
		if (!pitem) return;				// Safety

		if (pafter && gfxQueueGSyncIsInI(pqueue, pafter)) {
			pitem->next = pafter->next;
			pafter->next = pitem;
			if (pqueue->tail == pafter)
				pqueue->tail = pitem;
		} else {
			pitem->next = 0;
			if (!pqueue->head) {
				pqueue->head = pqueue->tail = pitem;
			} else {
				pqueue->tail->next = pitem;
				pqueue->tail = pitem;
			}
		}
	}

	void gfxQueueGSyncRemove(gfxQueueGSync *pqueue, gfxQueueGSyncItem *pitem) {
		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		gfxQueueGSyncRemoveI(pqueue, pitem);
		gfxSystemUnlock(&mux);
	}
	void gfxQueueGSyncRemoveI(gfxQueueGSync *pqueue, gfxQueueGSyncItem *pitem) {
		gfxQueueGSyncItem *pi;

		if (!pitem) return;				// Safety
		if (pqueue->head) {
			if (pqueue->head == pitem) {
				pqueue->head = pitem->next;
				pitem->next = 0;
			} else {
				for(pi = pqueue->head; pi->next; pi = pi->next) {
					if (pi->next == pitem) {
						pi->next = pitem->next;
						if (pqueue->tail == pitem)
							pqueue->tail = pi;
						pitem->next = 0;
						break;
					}
				}
			}
		}
	}

	bool_t gfxQueueGSyncIsIn(gfxQueueGSync *pqueue, const gfxQueueGSyncItem *pitem) {
		bool_t		res;

		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		res = gfxQueueGSyncIsInI(pqueue, pitem);
		gfxSystemUnlock(&mux);

		return res;
	}
	bool_t gfxQueueGSyncIsInI(gfxQueueGSync *pqueue, const gfxQueueGSyncItem *pitem) {
		gfxQueueGSyncItem *pi;

		for(pi = pqueue->head; pi; pi = pi->next) {
			if (pi == pitem)
				return TRUE;
		}
		return FALSE;
	}
#endif

#if GQUEUE_NEED_FSYNC
	void gfxQueueFSyncInit(gfxQueueFSync *pqueue) {
		pqueue->head = pqueue->tail = 0;
		gfxSemInit(&pqueue->sem, 0, MAX_SEMAPHORE_COUNT);
	}
	void gfxQueueFSyncDeinit(gfxQueueGSync *pqueue) {
		while(gfxQueueFSyncGet(pqueue, TIME_IMMEDIATE));
		pqueue->head = pqueue->tail = 0;
		gfxSemDestroy(&pqueue->sem);
	}

	gfxQueueFSyncItem *gfxQueueFSyncGet(gfxQueueFSync *pqueue, delaytime_t ms) {
		gfxQueueFSyncItem	*pi;

		if (!gfxSemWait(&pqueue->sem, ms))
			return 0;

		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		pi = pqueue->head;
		pqueue->head = pi->next;
		pi->next = 0;
		gfxSystemUnlock(&mux);

		gfxSemSignal(&pi->sem);
		gfxSemDestroy(&pi->sem);

		return pi;
	}

	bool_t gfxQueueFSyncPut(gfxQueueFSync *pqueue, gfxQueueFSyncItem *pitem, delaytime_t ms) {
		if (!pitem) return;				// Safety
		gfxSemInit(&pitem->sem, 0, 1);
		pitem->next = 0;

		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		if (!pqueue->head) {
			pqueue->head = pqueue->tail = pitem;
		} else {
			pqueue->tail->next = pitem;
			pqueue->tail = pitem;
		}
		gfxSystemUnlock(&mux);

		gfxSemSignal(&pqueue->sem);

		return gfxSemWait(&pitem->sem, ms);
	}

	bool_t gfxQueueFSyncPush(gfxQueueFSync *pqueue, gfxQueueFSyncItem *pitem, delaytime_t ms) {
		if (!pitem) return;				// Safety
		gfxSemInit(&pitem->sem, 0, 1);

		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		pitem->next = pqueue->head;
		pqueue->head = pitem;
		if (!pitem->next)
			pqueue->tail = pitem;
		gfxSystemUnlock(&mux);

		gfxSemSignal(&pqueue->sem);

		return gfxSemWait(&pitem->sem, ms);
	}

	bool_t gfxQueueFSyncInsert(gfxQueueFSync *pqueue, gfxQueueFSyncItem *pitem, gfxQueueASyncItem *pafter, delaytime_t ms) {
		if (!pitem) return;				// Safety
		gfxSemInit(&pitem->sem, 0, 1);

		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		if (pafter && gfxQueueGSyncIsInI(pqueue, pafter)) {
			pitem->next = pafter->next;
			pafter->next = pitem;
			if (pqueue->tail == pafter)
				pqueue->tail = pitem;
		} else {
			pitem->next = 0;
			if (!pqueue->head) {
				pqueue->head = pqueue->tail = pitem;
			} else {
				pqueue->tail->next = pitem;
				pqueue->tail = pitem;
			}
		}
		gfxSystemUnlock(&mux);

		gfxSemSignal(&pqueue->sem);

		return gfxSemWait(&pitem->sem, ms);

	}

	void gfxQueueFSyncRemove(gfxQueueFSync *pqueue, gfxQueueFSyncItem *pitem) {
		gfxQueueFSyncItem *pi;

		if (!pitem) return;				// Safety

		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		if (pqueue->head) {
			if (pqueue->head == pitem) {
				pqueue->head = pitem->next;
			found:
				pitem->next = 0;
				gfxSystemUnlock(&mux);
				gfxSemSignal(&pitem->sem);
				gfxSemDestroy(&pitem->sem);
				return;
			}
			for(pi = pqueue->head; pi->next; pi = pi->next) {
				if (pi->next == pitem) {
					pi->next = pitem->next;
					if (pqueue->tail == pitem)
						pqueue->tail = pi;
					goto found;
				}
			}
		}
		gfxSystemUnlock(&mux);
	}

	bool_t gfxQueueFSyncIsIn(gfxQueueFSync *pqueue, const gfxQueueFSyncItem *pitem) {
		bool_t	res;

		portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
		gfxSystemLock(&mux);
		res = gfxQueueFSyncIsInI(pqueue, pitem);
		gfxSystemUnlock(&mux);

		return res;
	}
	bool_t gfxQueueFSyncIsInI(gfxQueueFSync *pqueue, const gfxQueueFSyncItem *pitem) {
		gfxQueueASyncItem *pi;

		for(pi = pqueue->head; pi; pi = pi->next) {
			if (pi == pitem)
				return TRUE;
		}
		return FALSE;
	}
#endif

#if GQUEUE_NEED_BUFFERS
	bool_t gfxBufferAlloc(unsigned num, size_t size) {
		GDataBuffer *pd;

		if (num < 1)
			return FALSE;

		// Round up to a multiple of 4 to prevent problems with structure alignment
		size = (size + 3) & ~0x03;

		// Allocate the memory
		if (!(pd = gfxAlloc((size+sizeof(GDataBuffer)) * num)))
			return FALSE;

		// Add each of them to our free list
		for(;num--; pd = (GDataBuffer *)((char *)(pd+1)+size)) {
			pd->size = size;
			gfxBufferRelease(pd);
		}

		return TRUE;
	}

	void gfxBufferRelease(GDataBuffer *pd)		{ gfxQueueGSyncPut(&bufferFreeList, (gfxQueueGSyncItem *)pd); }
	void gfxBufferReleaseI(GDataBuffer *pd)		{ gfxQueueGSyncPutI(&bufferFreeList, (gfxQueueGSyncItem *)pd); }
	GDataBuffer *gfxBufferGet(delaytime_t ms)	{ return (GDataBuffer *)gfxQueueGSyncGet(&bufferFreeList, ms); }
	GDataBuffer *gfxBufferGetI(void)			{ return (GDataBuffer *)gfxQueueGSyncGetI(&bufferFreeList); }
	bool_t gfxBufferIsAvailable(void)			{ return bufferFreeList.head != 0; }

#endif


#endif /* GFX_USE_GQUEUE */
