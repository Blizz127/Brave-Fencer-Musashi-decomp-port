/* Owner re-derivation from retail assembly (clean input set). */
/* Classifies the low byte of the +0x3E half: 1 for several ids, 2 for 0xB, otherwise bit 1 of the high byte. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x3E];
    u16 f3E;
} Obj;

s32 func_80151880(Obj *o) {
    u16 v = o->f3E;
    u32 t = v & 0xFF;
    if (t == 0x1B) {
        return 1;
    }
    if (t == 0x1C) {
        return 1;
    }
    if (t == 0xF) {
        return 1;
    }
    if (t == 0x10) {
        return 1;
    }
    if (t == 0xB) {
        return 2;
    }
    if (t != 0x1F) {
        return (v >> 8) & 2;
    }
    return 1;
}
