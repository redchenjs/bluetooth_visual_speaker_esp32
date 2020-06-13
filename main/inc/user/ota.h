/*
 * ota.h
 *
 *  Created on: 2020-02-12 15:48
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_OTA_H_
#define INC_USER_OTA_H_

#include <stdint.h>

extern void ota_exec(const char *data, uint32_t len);
extern void ota_end(void);

#endif /* INC_USER_OTA_H_ */
