#include "system/init.h"
#include "tasks/main_task.h"

/**
 * app_main.c
 */
int app_main(void)
{
    device_init();
    driver_init(); 
     
    main_task();
    
    return 0;
}
