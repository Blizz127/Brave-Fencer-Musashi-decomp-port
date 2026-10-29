/* Owner re-derivation from retail assembly (clean input set). */
/* Marks matching active entries in an eight-slot table (stride 0x54) by id and optional second id. */
#include "psx_types.h"

extern u8 D_800A46E8[];

void func_800347C8(s32 arg) {
    s32 i;
    u8 *e;

    for (e = D_800A46E8, i = 0; i < 8; i++, e += 0x54) {
        if (*(u16 *)(e + 0) == 5 && *(u16 *)(e + 4) == (s16)arg) {
            if ((arg >> 16) == 0 || (arg >> 16) == *(u16 *)(e + 6)) {
                e[0x16] = 1;
            }
        }
    }
}
