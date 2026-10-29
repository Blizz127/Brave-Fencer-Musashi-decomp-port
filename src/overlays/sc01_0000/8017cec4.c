/* Owner re-derivation from retail assembly (clean input set). */
/* Counts down the s32 timer at offset 0x28; when it wraps to -1 plays two sound commands (0x1C, 0x1D), calls func_8001BFD0, func_800D0C48(1) and func_8017EB28, advances the u8 phase at offset 0x15; always returns 0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x15];
    u8 phase;
    u8 pad16[0x12];
    s32 timer;
} Obj;

extern void func_8002D4C8(s32 a, s32 b);
extern void func_8001BFD0(void);
extern void func_800D0C48(s32 a);
extern void func_8017EB28(void);

s32 func_8017CEC4(Obj *o)
{
    o->timer--;
    if (o->timer == -1) {
        func_8002D4C8(0x1C, 0);
        func_8001BFD0();
        func_8002D4C8(0x1D, 0);
        func_800D0C48(1);
        func_8017EB28();
        o->phase++;
    }
    return 0;
}
