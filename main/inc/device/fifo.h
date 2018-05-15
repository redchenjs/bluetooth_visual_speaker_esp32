/*
 * fifo.h
 *
 *  Created on: 2018-05-14 11:28
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_DEVICE_FIFO_H_
#define INC_DEVICE_FIFO_H_

#include <stdint.h>

typedef struct fifo_element {
    int16_t data;
    struct fifo_element *next;
} fifo_element_t;

extern void fifo_init(void);
extern void fifo_write(int16_t data);
extern int16_t fifo_read(void);

#endif /* INC_DEVICE_FIFO_H_ */
