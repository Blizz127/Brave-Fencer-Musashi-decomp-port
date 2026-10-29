/* func_8013AF20 [8013AF20,8013B204) - clean-room owner C, matched -O2.
 * Builds two 30-entry line strips and a 15-entry quad strip from three
 * func_80010A08 allocations and links each entry into the ordering table
 * word at ot. The 24-bit addr bitfield (link) and the i++, ptr++ increment
 * order (pointer giv set up first) reproduce retail's register allocation.
 * ((u8 *)prim)[3] is the tag length byte. */
#include "psx_types.h"

typedef struct {
    u32 addr : 24;
    u32 len : 8;
} PrimTag;

typedef struct {
    PrimTag tag;
    u8 r, g, b, code;
    s16 xy[4];
} LinePrim;

typedef struct {
    PrimTag tag;
    u8 r, g, b, code;
    s16 xy[8];
} QuadPrim;

extern void *func_80010A08(s32 size);

void func_8013AF20(PrimTag *ot, s16 *p2, s16 *p3, s8 *p4) {
    QuadPrim *quad;
    s32 i;
    LinePrim *b;
    LinePrim *p;

    quad = func_80010A08(0x168);
    p = func_80010A08(0x1E0);
    b = func_80010A08(0x1E0);

    for (i = 0; i < 30; i++, p++) {
        ((u8 *)p)[3] = 3;
        p->code = 0x40;
        p->r = 0x3D;
        p->g = 0x3D;
        p->b = 0x3D;
        p->xy[0] = p3[2 * i + 0];
        p->xy[1] = p3[2 * i + 1];
        p->xy[2] = p3[2 * i + 2];
        p->xy[3] = p3[2 * i + 3];
        p->tag.addr = ot->addr;
        ot->addr = (u32)p;
    }
    for (i = 0; i < 30; i++, b++) {
        ((u8 *)b)[3] = 3;
        b->code = 0x40;
        b->r = 0x92;
        b->g = 0x92;
        b->b = 0x92;
        b->xy[0] = p3[2 * i + 0] + p4[2 * i + 0];
        b->xy[1] = p3[2 * i + 1] + p4[2 * i + 1];
        b->xy[2] = p3[2 * i + 2] + p4[2 * i + 2];
        b->xy[3] = p3[2 * i + 3] + p4[2 * i + 3];
        b->tag.addr = ot->addr;
        ot->addr = (u32)b;
    }
    for (i = 0; i < 30; i += 2, quad++) {
        ((u8 *)quad)[3] = 5;
        quad->code = 0x28;
        quad->r = 0xFF;
        quad->g = 0xFF;
        quad->b = 0xFF;
        quad->xy[0] = p3[2 * i + 0];
        quad->xy[1] = p3[2 * i + 1];
        quad->xy[2] = p2[0];
        quad->xy[3] = p2[1];
        quad->xy[4] = p3[2 * i + 2];
        quad->xy[5] = p3[2 * i + 3];
        quad->xy[6] = p3[2 * i + 4];
        quad->xy[7] = p3[2 * i + 5];
        quad->tag.addr = ot->addr;
        ot->addr = (u32)quad;
    }
}
