/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct { u8 b[8]; } B8;

/* Returns retail's leftover v0: the u16 read from o + 0x2E (zero-extended). */
s32 func_8012A094(u8 *o) {
    s32 r;
    *(u32 *)(o + 8) = *(u32 *)(o + 0xC);
    *(u32 *)(o + 0x10) = *(u32 *)(o + 0x14);
    *(B8 *)(o + 0x18) = *(B8 *)(o + 0x20);
    r = *(u16 *)(o + 0x2E);
    *(u16 *)(o + 0x28) = r;
    *(u16 *)(o + 0x2A) = *(u16 *)(o + 0x30);
    *(u16 *)(o + 0x2C) = *(u16 *)(o + 0x32);
    return r;
}
