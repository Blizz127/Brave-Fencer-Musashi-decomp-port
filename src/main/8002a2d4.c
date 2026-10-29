/* Owner re-derivation from retail assembly (clean input set). */
/* If a level-up is due, shows the stat gain for the new level, plays the level-up sound and applies it; returns 1 when it happened. */
#include "psx_types.h"

typedef struct {
    u16 threshold;
    u8 pad2[0x10 - 2];
} LevelEntry;

extern s32 func_8002A354(s32 amount);
extern s32 func_8002A4B8(void);
extern void func_8002AAAC(void);
extern void func_8002D4C8(s32, s32);
extern void func_8016E918(s32, s16);
extern LevelEntry D_800638FE[];

/* Callers pass the amount in a0; it goes straight on to func_8002A354, which reads it. */
s32 func_8002A2D4(s32 amount) {
    s32 level;

    if (func_8002A354(amount) != 0) {
        level = func_8002A4B8();
        func_8016E918(1, D_800638FE[level].threshold - D_800638FE[level - 1].threshold);
        func_8002D4C8(0xB2C, 0);
        func_8002AAAC();
        return 1;
    }
    return 0;
}
