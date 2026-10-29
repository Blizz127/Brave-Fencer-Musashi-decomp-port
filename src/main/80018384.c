/* func_80018384: splice prim `p` into an ordering-table chain after `link`. */
#include "psx_types.h"

typedef struct {
    u32 addr : 24;
    u32 len : 8;
} PrimTag;

extern u32 D_800AE610;

void func_80018384(PrimTag *link, PrimTag *p) {
    if ((u32)&D_800AE610 < (u32)link) {
        for (;;) {
        }
    }
    p->addr = link->addr;
    link->addr = (u32)p;
}
