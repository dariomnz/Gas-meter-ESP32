#include <esp_log.h>
#include <esp_system.h>
#include "system.h"
#include <Arduino.h>

void disp_infos() {
    /* Print memory information */
    Serial.printf("esp_get_free_heap_size: %d\n", esp_get_free_heap_size());
    Serial.printf("esp_get_free_internal_heap_size: %d\n", esp_get_free_internal_heap_size());
    Serial.printf("esp_get_minimum_free_heap_size: %d\n", esp_get_minimum_free_heap_size());
}

static const int BMP_HEADER_LEN = 54;
typedef struct {
    uint32_t filesize;
    uint32_t reserved;
    uint32_t fileoffset_to_pixelarray;
    uint32_t dibheadersize;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsperpixel;
    uint32_t compression;
    uint32_t imagesize;
    uint32_t ypixelpermeter;
    uint32_t xpixelpermeter;
    uint32_t numcolorspallette;
    uint32_t mostimpcolor;
} bmp_header_t;

void *mi_malloc(size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void mi_free(void* ptr)
{
    return heap_caps_free(ptr);
}


bool camera2bmp(uint8_t *src, size_t src_len, uint16_t width, uint16_t height, uint16_t width_reduced, uint16_t height_reduced, pixformat_t format, uint8_t ** out, size_t * out_len)
{
    *out = NULL;
    *out_len = 0;
    uint16_t actual_width = width-width_reduced;
    uint16_t actual_height = height-height_reduced;
    int original_pix_count = width*height;
    int pix_count = actual_width*actual_height;
    size_t out_size = (pix_count * 3) + BMP_HEADER_LEN;
    uint8_t * out_buf = (uint8_t *)mi_malloc(out_size);
    if(!out_buf) {
        ESP_LOGE(TAG, "mi_malloc failed! %u", out_size);
        return false;
    }

    out_buf[0] = 'B';
    out_buf[1] = 'M';
    bmp_header_t * bitmap  = (bmp_header_t*)&out_buf[2];
    bitmap->reserved = 0;
    bitmap->filesize = out_size;
    bitmap->fileoffset_to_pixelarray = BMP_HEADER_LEN;
    bitmap->dibheadersize = 40;
    bitmap->width = actual_width;
    bitmap->height = -actual_height;//set negative for top to bottom
    bitmap->planes = 1;
    bitmap->bitsperpixel = 24;
    bitmap->compression = 0;
    bitmap->imagesize = pix_count * 3;
    bitmap->ypixelpermeter = 0x0B13 ; //2835 , 72 DPI
    bitmap->xpixelpermeter = 0x0B13 ; //2835 , 72 DPI
    bitmap->numcolorspallette = 0;
    bitmap->mostimpcolor = 0;

    uint8_t * rgb_buf = out_buf + BMP_HEADER_LEN;
    uint8_t * src_buf = &src[((original_pix_count*3)-(pix_count*3))/2];


    //convert data to RGB888
    if(format == PIXFORMAT_RGB888) {
        memcpy(rgb_buf, src_buf, pix_count*3);
    } else if(format == PIXFORMAT_RGB565) {
        int i;
        uint8_t hb, lb;
        for(i=0; i<pix_count; i++) {
            hb = *src_buf++;
            lb = *src_buf++;
            *rgb_buf++ = (lb & 0x1F) << 3;
            *rgb_buf++ = (hb & 0x07) << 5 | (lb & 0xE0) >> 3;
            *rgb_buf++ = hb & 0xF8;
        }
    } 
    *out = out_buf;
    *out_len = out_size;
    return true;
}