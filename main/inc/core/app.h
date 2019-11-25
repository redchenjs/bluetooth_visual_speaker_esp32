/*
 * app.h
 *
 *  Created on: 2018-04-05 19:09
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_CORE_APP_H_
#define INC_CORE_APP_H_

#include "esp_err.h"

extern const char *app_get_version(void);
extern void app_print_info(void);

extern esp_err_t app_getenv(const char *key, void *out_value, size_t *length);
extern esp_err_t app_setenv(const char *key, const void *value, size_t length);

#endif /* INC_CORE_APP_H_ */
