#ifndef MUSASHI_GPU_DITHER_H
#define MUSASHI_GPU_DITHER_H

/* Signed additions to 8-bit channels, indexed by absolute VRAM row and column. */
#define MUSASHI_GPU_DITHER_MATRIX { \
    {-4,  0, -3,  1},               \
    { 2, -2,  3, -1},               \
    {-3,  1, -4,  0},               \
    { 3, -1,  2, -2}                \
}

static inline unsigned musashi_gpu_quantize5(int channel, int offset) {
    int value = channel + offset;
    if (value < 0) return 0;
    if (value > 255) return 31;
    return (unsigned)value >> 3;
}

#endif
