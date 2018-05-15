/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "../../gfx.h"

#if GFX_USE_GMISC

#if GMISC_NEED_HITTEST_POLY

/* This function is used by gmiscHittestPoly()
 *
 * This function projects the point c on an horizontal line to the right.
 * If the projection cuts the segment formed by the points a and b,
 * the function returns 1. If the point c is on the segment, the function
 * returns 0. If they don't intersect, it returns 2.
 */
static char _pointCrossingSegment(const point *a, const point *b, const point *c) {
    /* If both points are left from our point, it won't intersect */
    if (a->x < c->x && b->x < c->x) {
        return -1;
    }

    /* Check if there is a point above and a point below, else
     * it won't intersect. 
     */
    if (c->y <= a->y && c->y >= b->y) {
        coord_t crossProduct;
        
        /* If the line is parallel */
        if (a->y == b->y) {
            /* Point on the segment */
            if ((c->x >= a->x && c->x <= b->x) || (c->x <= a->x && c->x >= b->x)) {
                return 0;
            } else {
                return -1;
			}
        }

        /* If the point is on the same horizontal line as one of the segment point,
         * allow to add the intersection once
         */
        if (c->y == b->y) {
            return -1;
		}

        /* Matrix cross product to find if the point is left or right from the segment*/
        crossProduct = (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);

        /* Point left of the segment */
        if (crossProduct < 0) {
            return 1;
		}

        /* Point on the segment */
        if (crossProduct == 0) {
            return 0;
		}
    }

    return -1;
}

bool_t gmiscHittestPoly(const point *pntarray, unsigned cnt, const point *p) {
    unsigned i = 0;
    uint8_t nbrIntersection = 0;
    int8_t crossResult;

    // For each pair of points
    for (i = 0; i < cnt-1; i++) {
        /* Order the two points from top to bottom to simplify the function */
        if (pntarray[i].y >= pntarray[i+1].y) {
            crossResult = _pointCrossingSegment(&pntarray[i], &pntarray[i+1], p);
        } else {
            crossResult = _pointCrossingSegment(&pntarray[i+1], &pntarray[i], p);
		}

        /* Point on the edge of the polygon */
        if (crossResult == 0) {
            return TRUE;
        }
        /* Point crossing the polygon */
        else if(crossResult == 1) {
            nbrIntersection++;
        }
    }

    /* Last pair of points, can't be done in the loops
     * Same as before
     */
    if (pntarray[cnt-1].y >= pntarray[0].y) {
        crossResult = _pointCrossingSegment(&pntarray[cnt-1], &pntarray[0], p);
    } else {
        crossResult = _pointCrossingSegment(&pntarray[0], &pntarray[cnt-1], p);
    }

    if (crossResult == 0) {
        return TRUE;
    } else if(crossResult == 1) {
        nbrIntersection++;
    }

    /* If we cross an even pair of segments, we are outside */
    if (nbrIntersection % 2 == 0) {
        return FALSE;
    }
    
    /* Else we are inside the polygon */
    return TRUE;
}

#endif // GMISC_NEED_HITTEST_POLY

#endif // GFX_USE_GMISC
