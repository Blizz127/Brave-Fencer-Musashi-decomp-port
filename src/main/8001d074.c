/* Owner re-derivation from retail assembly (clean input set). */
/* Scans table entries [first,last) of D_800AF630 (0x38 bytes each) and returns the address of the first one whose halfword field at +0x65A8 is zero, or 0. */
#include "psx_types.h"
extern u8 D_800AF630[];
u16 *func_8001D074(s32 first, s32 last) {
    u8 *b = D_800AF630;
    if ((u32)first >= 0x100) return 0;
    if ((u32)last >= 0x101) return 0;
    while (first < last) {
        s32 off = first * 0x38;
        u8 *e = b + off;
        if (*(u16 *)(e + 0x65A8) == 0) return (u16 *)(b + (off + 0x65A8));
        first++;
    }
    return 0;
}
