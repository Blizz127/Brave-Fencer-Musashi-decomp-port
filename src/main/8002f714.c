/* Owner re-derivation from retail assembly (clean input set). */
/* Requests a volume change on one of the four mix buses and arms the bus-update callback. */
#include "psx_types.h"

typedef struct {
    u8 level;
    u8 pad1[3];
} MixBus;

extern void func_8002F80C(void);
extern u8 D_8006A96E;
extern MixBus D_800760D0[4];
extern void (*D_800A4F24)(void);

void func_8002F714(u16 bus, s32 level) {
    switch (bus) {
    case 0:
        D_800760D0[0].level = level & 0x7F;
        D_800A4F24 = func_8002F80C;
        D_8006A96E |= 1;
        break;
    case 1:
        D_800760D0[1].level = level & 0x7F;
        D_800A4F24 = func_8002F80C;
        D_8006A96E |= 2;
        break;
    case 2:
        D_800760D0[2].level = level & 0x7F;
        D_800A4F24 = func_8002F80C;
        D_8006A96E |= 4;
        break;
    case 3:
        D_800760D0[3].level = level & 0x7F;
        D_800A4F24 = func_8002F80C;
        D_8006A96E |= 8;
        break;
    }
}
