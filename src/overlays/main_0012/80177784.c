/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
typedef struct {
    u32 tag;
    u32 col;
    s32 a;
    s32 b;
} Prim;

Prim *func_80177784(Prim *p, s32 a, s32 b) {
    p->tag = (((u32)p - 0x14) & 0xFFFFFF) | 0x3000000;
    p->col = 0x74808080;
    p->a = a;
    p->b = b;
    return (Prim *)((u8 *)p + 0x14);
}
