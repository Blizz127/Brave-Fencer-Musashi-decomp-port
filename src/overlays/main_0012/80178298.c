#include "psx_types.h"
typedef struct { u32 tag; u32 rgb; s16 x; s16 y; u8 u; u8 v; u16 clut; u16 w; u16 h; } Sprt;
Sprt *func_80178298(Sprt *p, u8 *s, s16 x, s16 y) {
    while (*s < 0x80) {
        switch (*(s16 *)s) {
        case 0x1850:
        case 0x1858:
            p->x = x + 4;
            p->y = y - 7;
            break;
        case 0x3870:
            x += 8;
            s += 2;
            continue;
        case 0x3871:
            x += 4;
            s += 2;
            continue;
        case 0x3872:
            x += 2;
            s += 2;
            continue;
        default:
            p->x = x;
            p->y = y;
            if (*(s16 *)s >= 0x861) {
                x += 8;
            } else {
                x += 7;
            }
            break;
        }
        p->tag = (((u32)p - 0x14) & 0xFFFFFF) | 0x04000000;
        p->rgb = 0x64808080;
        p->u = *s++;
        p->v = *s++;
        p->clut = 0x4056;
        p->w = 8;
        p->h = 8;
        p++;
    }
    return p;
}
