/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* For each of the 16 channels that is idle (state 0 or 3) and not held, clears its busy flag and, if it was keyed, clears its key byte in the owning record and the keyed flag. */
/* Verified 42/42 words with the pinned oracle, gcc-2.7.2-psx / ASPSX 2.56, -O2 -G0. */
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

extern u8 *D_800762B0;
extern u8 D_800762B4[];
extern Channel D_800C6DD0[];

void func_80038958(void) {
    u8 *state;
    u8 *hold;
    Channel *ch;
    s32 i;
    u8 *rec;

    state = D_800762B0;
    hold = D_800762B4;
    i = 0;
    ch = D_800C6DD0;
    do {
        if ((*state == 0 || *state == 3) && *hold == 0) {
            ch->busy = 0;
            if (ch->keyed != 0) {
                rec = (u8 *)ch->records + ch->record * 0x1A + i;
                rec[0x23] = 0;
                ch->keyed = 0;
            }
        }
        i++;
        ch++;
        state++;
        hold++;
    } while (i < 16);
}
