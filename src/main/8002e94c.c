/* Owner re-derivation from retail assembly (clean input set). */
/* Services two pending-request flag bytes: fires func_8002EAB0 on a matching state, then consumes a queued request and loads the table values for its index. */
#include "psx_types.h"

extern u16 D_800A4E8E;
extern u16 D_800A4E8C;
extern u8 D_800A4EE6;
extern u16 D_800A4EE4;
extern u16 D_800A4EE2;
extern u16 D_8006A9EC[];
extern void func_8002EAB0(void);

void func_8002E94C(u16 idx) {
    u32 flags = D_800A4E8E;
    u8 *state;
    u32 cur;
    u32 rest;
    u16 val;

    if ((flags & 4) && D_800A4E8C == 0x3E && (flags & 0x10)) {
        func_8002EAB0();
    }

    state = &D_800A4EE6;
    cur = *state;
    if (cur & 4) {
        rest = cur & 0xFB;
        *state = rest;
        if (!(cur & 2)) {
            if (idx >= 5) {
                idx = 4;
            }
            D_800A4EE4 = 0x4000;
            val = D_8006A9EC[idx];
            *state = rest | 1;
            D_800A4EE2 = val;
        }
    }
}
