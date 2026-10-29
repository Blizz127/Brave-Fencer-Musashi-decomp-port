/* Owner re-derivation from retail assembly (clean input set). */
/* Emits two 0x14-byte tile primitives (a colored marker and a gauge) chained to the previous primitive; returns the next free slot. */
#include "psx_types.h"

typedef struct {
    u32 tag;
    u8 r;
    u8 g;
    u8 b;
    u8 code;
    s16 x;
    s16 y;
    s16 w;
    s16 h;
    u32 pad;
} P;

typedef struct { u8 v; } B1;
extern u8 D_8011F7AB;
extern u8 D_8011F7F0[];
extern u8 D_8011F837;

P *func_8017742C(P *p, s16 x, s16 y) {
    P *q = p;
    u8 *d = D_8011F7F0;
    s32 h;
    p->tag = (((u32)(p - 1)) & 0xFFFFFF) | 0x3000000;
    p->code = 0x60;
    if (D_8011F837 < 0x80) {
        p->r = 0xFF;
        p->b = 0x10;
        p->g = 0x10;
    } else if (D_8011F837 == 0x80) {
        switch (D_8011F7AB) {
        default:
            D_8011F7AB = 0;
        case 0:
            q->r = 0xFF;
            q->b = 0x30;
            q->g = 0x30;
            break;
        case 1:
            p->g = 0xFF;
            p->b = 0x30;
            p->r = 0x30;
            break;
        case 2:
            p->b = 0xFF;
            p->g = 0x30;
            p->r = 0x30;
            break;
        }
        ((B1 *)&D_8011F7AB)->v++;
    } else {
        p->g = 0xFF;
        p->b = 0x10;
        p->r = 0x10;
    }
    p++;
    q->x = x;
    h = (d[0x47] * 3) >> 3;
    q->w = 8;
    q->y = (s16)(y + 0x48) - h;
    q->h = h;
    p->tag = (((u32)(p - 1)) & 0xFFFFFF) | 0x3000000;
    p->code = 0x60;
    p->r = p->g = 0x40;
    q = p;
    if (d[0x47] != 0) {
        p->b = 0xA0;
    } else {
        p->b = 0x40;
    }
    q->w = 8;
    q->h = 0x48;
    q->x = x;
    q->y = y;
    return p + 1;
}
