/* Owner re-derivation from retail assembly (clean input set). */
/* Predicate on an entry p with state k and mask: false for inactive entries, true for the special kinds, else whether it is not the current one. */
#include "psx_types.h"
typedef struct { u16 f0; u8 pad2[0x56]; u32 f58; u32 f5C; u8 pad60[0x4E]; s16 fAE; } P;
typedef struct { u8 pad[0x178]; u32 f178; } E;
s32 func_8014C308(E *e, P *p, s32 k, u16 mask) {
    if (p->f0 == 0) return 0;
    if (p->f58 == 0) return 0;
    if (((u16 *)&p->f5C)[1] != k && (p->f5C & 0xC100) == 0x8000 && (p->fAE & mask) == 0) {
        if (k != 9 && k != 0x11 && k != 0x29 && k != 0xA && e->f178 == (u32)p) return 0;
        return 1;
    }
    return 0;
}
