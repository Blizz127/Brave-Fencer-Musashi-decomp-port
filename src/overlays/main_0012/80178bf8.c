/* Owner re-derivation from retail assembly (clean input set). */
/* Resets the scene-script state: clears the status words and shorts, the ten-entry callback table
 * D_80185394 and the 0x200-entry short buffer D_80184F40 (both walked downward), clears the two
 * position shorts, registers the 0x14-sized region D_80185BD8 with func_80016714 and runs
 * func_8017B1D8. D_80185B24 is set to 1. */
#include "psx_types.h"

extern s16 D_80185C7C;
extern s32 D_80185350;
extern u16 D_80185360;
extern u16 D_8018535C;
extern u16 D_80185358;
extern u16 D_80185354;
extern s32 D_80185B24;
extern s32 D_80185368;
extern s16 D_80185348;
extern u32 D_80185394[10];
extern u16 D_80184F40[0x200];
extern u16 D_80185340;
extern u16 D_80185344;
extern u8 D_80185BD8[];
extern void func_80016714(u8 *p, s32 n);
extern void func_8017B1D8(void);

void func_80178BF8(void)
{
    s32 i;

    D_80185C7C = 0;
    D_80185350 = 0;
    D_80185360 = 0;
    D_8018535C = 0;
    D_80185358 = 0;
    D_80185354 = 0;
    D_80185B24 = 1;
    D_80185368 = 0;
    D_80185348 = 0;
    for (i = 9; i >= 0; i--) {
        D_80185394[i] = 0;
    }
    for (i = 0x1FF; i >= 0; i--) {
        D_80184F40[i] = 0;
    }
    D_80185340 = 0;
    D_80185344 = 0;
    func_80016714(D_80185BD8, 0x14);
    func_8017B1D8();
}
