#pragma once

#include "../../../gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

bool_t qimage_init(GDisplay* g, coord_t width, coord_t height);
void qimage_setPixel(GDisplay* g);
color_t qimage_getPixel(GDisplay* g);

#ifdef __cplusplus
}
#endif
