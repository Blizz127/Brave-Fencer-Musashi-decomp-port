/* func_80155D70 [80155D70,80155E30) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

/* callers read v0: none-direct (no jal caller); retail sets no v0 of its own. */
extern u16 func_801487F4(u8 *o);
extern void func_801599A4(void *o);
extern void func_8015E880(u8 *o);
extern void func_80159B3C(void *o);
extern s32 func_8014CC28(u8 *o);
extern int func_8014F3E8(u8 *o);
extern void func_8015BDD0(void *o);

void func_80155D70(u8 *o) {
    s32 pad[16];
    s32 r;

    *(u32 *)(o + 0x44) |= 2;
    if (*(u16 *)(o + 0xB8) == 0x8000) {
        if ((func_801487F4(o) & 0x80) != 0) {
            func_801599A4(o);
            func_8015E880(o);
        } else {
            func_801599A4(o);
            func_80159B3C(o);
        }
    } else {
        r = func_8014CC28(o);
        if (func_8014F3E8(o) == 0 && r == 0) {
            func_801599A4(o);
            func_8015BDD0(o);
        }
    }
}
