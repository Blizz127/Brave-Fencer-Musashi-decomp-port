/* Owner re-derivation from retail assembly (clean input set). */
/* Syncs the linked node (+0x20) with this object: copies positions plus offsets (+6/+0x50, +0xA/+0x52, +0xE/+0x54) into it and flags it dirty. */
#include "psx_types.h"
void func_80049CAC(int a, int b);
void func_8012B2CC(char *o) {
    int n = *(int *)(o + 0x20);
    if (n != 0) {
        char *q;
        u16 x;
        func_80049CAC(n + 0x10, n + 0x34);
        q = (char *)*(int *)(o + 0x20);
        x = *(u16 *)(o + 6) + *(u16 *)(o + 0x50);
        *(u16 *)(q + 8) = x;
        *(int *)(q + 0x48) = (short)x;
        x = *(u16 *)(o + 0xA) + *(u16 *)(o + 0x52);
        *(u16 *)(q + 0xA) = x;
        *(int *)(q + 0x4C) = (short)x;
        x = *(u16 *)(o + 0xE) + *(u16 *)(o + 0x54);
        *(u16 *)(q + 0xC) = x;
        *(u16 *)(q + 0x2C) |= 1;
        *(int *)(q + 0x50) = (short)x;
    }
}
