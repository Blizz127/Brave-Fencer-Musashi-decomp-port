/* Owner re-derivation from retail assembly (clean input set). */
/* Copies a record's header bytes into its mirror area (first two bytes inverted and swapped), or defers to func_80018F88 when byte 8 is set. */
#include "psx_types.h"

extern void func_80018F88(u8 *, s32);

void func_80018C64(u8 *rec) {
    s32 i;

    i = 0;
    if (rec[8] != 0) {
        func_80018F88(rec, i);
        return;
    }
    rec[0] = rec[9];
    for (i = 0; i < 2; i++) {
        rec[0x2B - i] = ~(rec + i)[0xA];
    }
    for (i = 2; i < 6; i++) {
        (rec + i)[0x2A] = (rec + i)[0xA];
    }
}
