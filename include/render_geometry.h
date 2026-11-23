#ifndef BFM_RENDER_GEOMETRY_H
#define BFM_RENDER_GEOMETRY_H

#include "psx_types.h"

typedef struct {
    u32 xy; /* vx | vy << 16 */
    s16 z;
    s16 pad;
} QuadVert;

typedef struct {
    QuadVert v[4];
    u32 rgb[4];
    u32 flags;
} QuadDesc;

void func_80017778(QuadDesc *quad, void *matrix, s32 mode);
void func_80020DA4(u16 *angles, s16 *matrix);

#endif
