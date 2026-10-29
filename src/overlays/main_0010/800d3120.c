/* Owner re-derivation from retail assembly (clean input set). */
/* State handler for a record: counts the byte at +0x19 down and, when it was already 0, plays the two sound commands, runs func_800D0C48(1) and func_800167B8(4), then advances the state byte at +0x15. Always returns 0. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x15];
    u8 state;
    u8 pad16[3];
    u8 timer;
} Rec800D3120;

extern void func_8002D4C8(s32 a, s32 b);
extern void func_8001BFD0(void);
extern void func_800D0C48(s32 a);
extern void func_800167B8(s32 a);

s32 func_800D3120(Rec800D3120 *r)
{
    u8 t;

    t = r->timer;
    r->timer = t + 0xFF;
    if (t == 0) {
        func_8002D4C8(0x1C, 0);
        func_8001BFD0();
        func_8002D4C8(0x1D, 0);
        func_800D0C48(1);
        func_800167B8(4);
        r->state = r->state + 1;
    }
    return 0;
}
