#ifndef MUSASHI_FOREST_FLOOR_H
#define MUSASHI_FOREST_FLOOR_H

#include "psx_types.h"

typedef struct {
    u16 x, y, z;
    s16 w;
} FloorPoint;

typedef struct {
    s16 v, d;
} FloorPlane;

typedef struct {
    u16 flags;
    /* Four entries for a short face and five for a long face. */
#if defined(__GNUC__) && __GNUC__ < 3
    FloorPlane e[0];
#else
    FloorPlane e[];
#endif
} FloorFace;

typedef struct {
    u16 x0, z0, w, h;
    u16 *cells;
    s32 *dist;
    FloorPoint *verts;
    u8 *ids;
    u8 *faces18;
    u8 *faces22;
} FloorGrid;

extern FloorPoint *D_801870AC;
extern FloorPoint *D_801870B0;
extern FloorPoint *D_801870B4;
extern FloorPoint D_801152A8;
extern FloorGrid *D_801D94F0;
extern u16 D_801D9500;

u32 func_80134510(FloorPoint *point);
s32 func_801345F8(FloorGrid *grid, FloorPoint *start, FloorPoint *end);
s32 func_801347A0(s16 five_planes, FloorFace *face, FloorPoint *vertices, s32 *distances);
s32 func_80134A28(FloorPoint *normal, FloorPoint *point, s32 distance);

extern FloorPoint *D_801870B8;
s32 func_80134A74(s32 flags, u16 x, u16 z, FloorGrid *grid);
s32 func_80134C20(s16 flags, FloorFace *face, FloorPoint *vertices, s32 *distances);
s32 func_80134FB8(FloorPoint *normal, FloorPoint *point, s32 distance);

#endif
