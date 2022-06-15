#ifndef __SYSTEM_H
#define __SYSTEM_H

#include "sensor.h"

// Display memory info
void disp_infos();

void *mi_malloc(size_t size);
void mi_free(void* ptr);

bool camera2bmp(uint8_t *src, size_t src_len, uint16_t width, uint16_t height, uint16_t width_reduced, uint16_t height_reduced, pixformat_t format, uint8_t ** out, size_t * out_len);

#endif
