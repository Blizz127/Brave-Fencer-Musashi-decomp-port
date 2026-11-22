/* Owner re-derivation from retail assembly (clean input set). */
/* Initialises a handful of global settings and registers six flags through a helper. */
#include "event_flags.h"

extern s16 D_80078EB2;
extern s16 D_80078EB4;
extern s16 D_80078EB6;
extern s16 D_80078EB8;
extern u8 D_80078EC0;
extern s32 D_80078E8C;

void func_800295D4(void) {
    D_80078EB4 = 0xFA;
    D_80078EB2 = 0xFA;
    D_80078EB8 = 0xFA;
    D_80078EB6 = 0xFA;
    func_80029124(0x1E, 1);
    D_80078EC0 = 1;
    func_80029124(0x80, 1);
    D_80078E8C = 0x64;
    func_80029124(0x82, 1);
    func_80029124(0x83, 1);
    func_80029124(0x85, 1);
}
