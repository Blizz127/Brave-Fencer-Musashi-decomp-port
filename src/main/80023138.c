/* Owner re-derivation from retail assembly (clean input set). */
/* Builds a 12-segment Gouraud ring (POLY_G4 strip between an outer and inner point loop) into the primitive buffer and links it into the ordering table, with optional semi-transparency mode packets. */
#include "psx_types.h"

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

typedef struct {
    PrimTag tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 r1, g1, b1, pad1;
    s16 x1, y1;
    u8 r2, g2, b2, pad2;
    s16 x2, y2;
    u8 r3, g3, b3, pad3;
    s16 x3, y3;
} PolyG4;

typedef struct {
    PrimTag tag;
    u32 code;
} DrMode;

typedef struct {
    s32 pad0;
    s16 inner[24];
    s32 depth[6];
    s16 outer[26];
} RingShape;

extern u8 *D_800A5E60;

void func_80023138(u32 *obj, u8 *colors, RingShape *shape, PrimTag *ot) {
    PolyG4 *poly;
    DrMode *mode;
    s32 i;
    u32 flags;
    u8 code;
    s32 unused_frame[12]; /* retail reserves an unused 48-byte frame */

    poly = (PolyG4 *)D_800A5E60;
    flags = obj[1];
    D_800A5E60 = (u8 *)(poly + 12);
    code = 0x38;
    if (flags & 0x40000000) {
        code = 0x3A;
    }
    for (i = 0; i < 12; i++, poly++) {
        poly->tag.len = 8;
        poly->code = code;
        poly->r0 = colors[4];
        poly->g0 = colors[5];
        poly->b0 = colors[6];
        poly->r1 = colors[4];
        poly->g1 = colors[5];
        poly->b1 = colors[6];
        poly->r2 = colors[0];
        poly->g2 = colors[1];
        poly->b2 = colors[2];
        poly->r3 = colors[0];
        poly->g3 = colors[1];
        poly->b3 = colors[2];
        poly->x0 = shape->outer[i * 2];
        poly->y0 = shape->outer[i * 2 + 1];
        poly->x1 = shape->outer[i * 2 + 2];
        poly->y1 = shape->outer[i * 2 + 3];
        poly->x2 = shape->inner[i * 2];
        poly->y2 = shape->inner[i * 2 + 1];
        poly->x3 = shape->inner[i * 2 + 2];
        poly->y3 = shape->inner[i * 2 + 3];
        poly->tag.addr = ot[shape->depth[i >> 1]].addr;
        ot[shape->depth[i >> 1]].addr = (u32)poly;
    }
    poly[-1].x1 = shape->outer[0];
    poly[-1].y1 = shape->outer[1];
    poly[-1].x3 = shape->inner[0];
    poly[-1].y3 = shape->inner[1];
    if (flags & 0x40000000) {
        mode = (DrMode *)poly;
        D_800A5E60 += 12 * 8; /* 12 DrMode of 8 bytes (guest and host); the native lane refuses sizeof */
        for (i = 0; i < 12; i++, mode++) {
            mode->tag.len = 1;
            mode->code = 0xE1000000 | ((flags >> 23) & 0x60);
            mode->tag.addr = ot[shape->depth[i >> 1]].addr;
            ot[shape->depth[i >> 1]].addr = (u32)mode;
        }
    }
}
