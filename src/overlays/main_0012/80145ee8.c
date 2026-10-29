/* Owner re-derivation from retail assembly (clean input set). */
/* Registers the input-binding tables of the pad. Mode 0 adds a table for each of the pad bits
 * 0x1F, 0x21, 0x1E, 0x1D and 0x20 that func_80029178 reports as held; mode 1 adds all five
 * unconditionally; other modes do nothing. */
#include "psx_types.h"

extern u32 D_8017FD40[];
extern u32 D_8017FD70[];
extern u32 D_8017FDC0[];
extern u32 D_8017FDF0[];
extern u32 D_8017FE20[];
extern s32 func_80029178(s32 bit);
extern void func_80146014(u32 *table);

void func_80145EE8(s32 mode)
{
    switch (mode) {
    case 0:
        if ((u8)func_80029178(0x1F)) {
            func_80146014(D_8017FD40);
        }
        if ((u8)func_80029178(0x21)) {
            func_80146014(D_8017FD70);
        }
        if ((u8)func_80029178(0x1E)) {
            func_80146014(D_8017FDC0);
        }
        if ((u8)func_80029178(0x1D)) {
            func_80146014(D_8017FDF0);
        }
        if ((u8)func_80029178(0x20)) {
            goto last;
        }
        break;
    case 1:
        func_80146014(D_8017FD40);
        func_80146014(D_8017FD70);
        func_80146014(D_8017FDC0);
        func_80146014(D_8017FDF0);
    last:
        func_80146014(D_8017FE20);
        break;
    }
}
