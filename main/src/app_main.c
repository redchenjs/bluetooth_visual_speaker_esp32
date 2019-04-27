/*
 * app_main.c
 *
 *  Created on: 2018-03-11 15:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "os/init.h"

int app_main(void)
{
    os_init();          // OS Event
    chip_init();        // OnChip Module
    board_init();       // OnBoard Module
    user_init();        // User Task

    return 0;
}
