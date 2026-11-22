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
s32 func_80134A74(s32 flags, s16 x, s16 z, FloorGrid *grid);
s32 func_80134C20(s16 flags, FloorFace *face, FloorPoint *vertices, s32 *distances);
s32 func_80134FB8(FloorPoint *normal, FloorPoint *point, s32 distance);

s32 func_80135004(u16 flags, FloorPoint *from, FloorPoint *to);

s32 func_801638A0(u8 *actor, FloorPoint *endpoint);

/* The rotation occupies five words at +0x34. The translation follows at +0x48. */
typedef union {
    s16 coefficients[10];
    s32 pairs[5];
} FloorRotation;

typedef struct {
    u8 before_scale[0x18];
    s16 scale_x, scale_y, scale_z;
    u8 before_rotation[0x16];
    FloorRotation rotation;
    u32 x, y, z;
} FloorContactRecord;

typedef struct {
    FloorRotation rotation;
    u32 x, y, z;
} FloorMatrix;

typedef struct {
    u32 next;
    s16 xlo, xhi, ylo, yhi, zlo, zhi;
} FloorBox;

extern FloorMatrix D_801D9504;
extern FloorBox D_801D9524;
s32 func_80135480(FloorContactRecord *record, u32 packed, FloorPoint *near_point, FloorPoint *far_point);

s32 func_80135EB0(FloorBox *box, s32 flags);
s32 func_80136334(FloorBox *box, s32 flags, s16 z);
s32 func_801365B8(FloorBox *box, s32 flags, s16 x);
s32 func_80136824(FloorBox *box, s32 flags, s16 y);
s32 func_80135888(FloorContactRecord *record, s32 packed, FloorPoint *start, FloorPoint *end);
s32 func_80135A4C(FloorContactRecord *record, s32 packed, FloorPoint *start, FloorPoint *end);
s32 func_80133784(s16 mode, FloorPoint *first, FloorPoint *last);
s32 func_80133AB0(s16 flags, s16 x, s16 z, FloorGrid *grid);
s32 func_801343C4(s16 mode, FloorPoint *start, FloorPoint *end);
s16 func_80133CD4(s16 flags, FloorFace *face, FloorPoint *vertices, s32 *distances);
extern s16 D_801D94FC;
extern FloorPoint D_801152B0;
#ifndef __mips__
/* Resolve box handles to FloorBox and grid handles to widened FloorGrid. Zero ends a list. */
void *bfm_forest_contact_model(u32 address, s32 is_box);
#endif

extern FloorPoint D_80126720;
void func_80136A94(s32 rotate, FloorContactRecord *record, FloorPoint *output, void *matrix);

#endif
