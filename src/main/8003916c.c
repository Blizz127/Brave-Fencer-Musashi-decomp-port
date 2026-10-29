/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* If a channel is keyed, clears its key byte in the owning record and clears the keyed flag. */
#include "psx_types.h"

typedef struct {
    u8 pad0[6];
    s16 record;
    u8 pad8[0x50 - 8];
    u32 records; /* guest u8 * */
    u8 pad54[0x5A - 0x54];
    u8 keyed;
    u8 busy;
    u8 pad5C[0x60 - 0x5C];
} Channel;

extern Channel D_800C6DD0[];

void func_8003916C(s16 index) {
    Channel *ch;
    u8 *rec;

    ch = &D_800C6DD0[index];
    if (ch->keyed != 0) {
        rec = (u8 *)ch->records + ch->record * 0x1A + index;
        rec[0x23] = 0;
        ch->keyed = 0;
    }
}
