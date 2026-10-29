/* Owner re-derivation from retail assembly (clean input set). */
/* Derives the day-phase index, the daytime flag and the hour/minute display bytes from the in-game clock. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x37];
    u8 phase;
    u8 minutes;
    u8 hours;
} AreaClock;

extern AreaClock D_80078E78;
extern u16 D_80078E92;
extern s8 D_80078EAE;

void func_80029344(void) {
    AreaClock *clock;
    s16 time;

    time = D_80078E92;
    clock = &D_80078E78;
    if ((u32)((u16)time - 420) >= 720) {
        D_80078EAE = 0;
    } else {
        D_80078EAE = 1;
    }
    if (time < 1320) {
        if (time >= 1140) {
            clock->phase = 3;
        } else if (time >= 960) {
            clock->phase = 2;
        } else if (time >= 420) {
            clock->phase = 1;
        } else if (time >= 300) {
            clock->phase = 0;
        } else {
            clock->phase = 4;
        }
    } else {
        clock->phase = 4;
    }
    clock->hours = time / 60;
    clock->minutes = time % 60;
}
