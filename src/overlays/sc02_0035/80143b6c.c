/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad[0x20];
    u32 link;
} O;

typedef struct {
    u8 pad[0xFC];
    u16 f;
} R;

typedef struct {
    u8 pad[0x12];
    u16 h;
} L;

extern R *func_8012C658(s32 a, s32 b, O *o);

R *func_80143B6C(O *o, s32 flag) {
    R *r;
    r = func_8012C658(0x1C, flag, o);
    if (r != 0 && o->link != 0 && flag == 0) {
        r->f = ((L *)o->link)->h;
    }
    return r;
}