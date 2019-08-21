#ifndef RENDER_H
#define RENDER_H
#include <stdint.h>
enum texture_type {
    TEX_RGBA8 = 0,
    TEX_RGB,
    TEX_RGBA4,
    TEX_RGB565,
    TEX_A8,
    TEX_A4
};

struct texture {
    int fmt;
    int w;
    int h;
    uint8_t *data;
};

#endif