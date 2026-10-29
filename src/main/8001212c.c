/* Owner re-derivation from retail assembly (clean input set). */
/* Adds two textured sprite packets (the two halves of a menu caption) to this frame's primitive buffer and ordering table. */
#include "psx_types.h"

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

typedef struct {
    PrimTag tag;
    u32 mode;
    u8 r, g, b, code;
    s16 x, y;
    u8 u, v;
    u16 clut;
    s16 w, h;
} ModeSprite;

extern ModeSprite *D_800A5E60;
extern PrimTag D_800A6610[][4096];
extern u8 D_800AF630[];

void func_8001212C(void) {
    ModeSprite *p;
    PrimTag *ot;
    register u8 *state = D_800AF630;

    p = D_800A5E60;
    ot = D_800A6610[*(u16 *)(state + 0xA3D2)];

    p->tag.len = 5;
    ((u32 *)p)[1] = 0xE1000018;
    p->code = 0x66;
    p->clut = 0x77D6;
    p->r = 0x80;
    p->g = 0x80;
    p->b = 0x80;
    p->x = 48;
    p->y = 46;
    p->u = 64;
    p->v = 224;
    p->w = 96;
    p->h = 24;
    p->tag.addr = ot[1].addr;
    ot[1].addr = (u32)p;
    p++;

    p->tag.len = 5;
    ((u32 *)p)[1] = 0xE1000018;
    p->code = 0x66;
    p->clut = 0x77D6;
    p->r = 0x80;
    p->g = 0x80;
    p->b = 0x80;
    p->x = 48;
    p->y = 70;
    p->u = 160;
    p->v = 224;
    p->w = 96;
    p->h = 24;
    p->tag.addr = ot[1].addr;
    ot[1].addr = (u32)p;
    p++;

    D_800A5E60 = p;
}
