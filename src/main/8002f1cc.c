/* Owner re-derivation from retail assembly (clean input set). */
/* Reactivates up to eight entries of a record when their flag byte is set. */
#include "psx_types.h"

typedef struct Ent54 {
    u8 pad0[0x20];
    s32 f20;
    u8 pad24[4];
    s32 f28;
    s32 f2C;
    s16 f30;
    u8 pad32[5];
    u8 f37;
    u8 f38;
    u8 pad39[0x54 - 0x39];
} Ent54;

extern u8 D_800A4694[];

void func_8002F1CC(s32 arg0) {
    u8 *rec;
    Ent54 *arr;
    s32 i;

    rec = D_800A4694 + arg0 * 0x54;
    arr = (Ent54 *)(D_800A4694 + 0x2F4);
    for (i = 0; i < 8; i++) {
        if (*(rec + i + 0xE) != 0) {
            arr[i].f30 = 0;
            arr[i].f38 = 0;
            arr[i].f2C = 0x18;
            arr[i].f28 = arr[i].f20 + 0x100;
            arr[i].f37 |= 1;
        }
    }
}
