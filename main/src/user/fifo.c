/*
 * fifo.c
 *
 *  Created on: 2018-05-14 11:28
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "user/fifo.h"

#define FIFO_SIZE 128

uint16_t fifo_buf_num = 0;
fifo_element_t fifo_buf[FIFO_SIZE] = {0};
fifo_element_t *fifo_next_write = NULL;
fifo_element_t *fifo_next_read  = NULL;

void fifo_init(void)
{
    uint16_t i = 0;
    for (i=0; i<FIFO_SIZE-1; i++) {
        fifo_buf[i].data = 0;
        fifo_buf[i].next = &fifo_buf[i+1];
    }
    fifo_buf[i].data = 0;
    fifo_buf[i].next = &fifo_buf[0];
    fifo_next_write = &fifo_buf[0];
    fifo_next_read  = &fifo_buf[0];
}

void fifo_write(int16_t data)
{
    if (fifo_buf_num++ == FIFO_SIZE) {
        fifo_buf_num = FIFO_SIZE;
    }
    fifo_next_write->data = data;
    fifo_next_write = fifo_next_write->next;
}

int16_t fifo_read(void)
{
    if (fifo_buf_num-- == 0) {
        fifo_buf_num = 0;
        return 0x0000;
    }
    int16_t data = fifo_next_read->data;
    fifo_next_read = fifo_next_read->next;
    return data;
}
