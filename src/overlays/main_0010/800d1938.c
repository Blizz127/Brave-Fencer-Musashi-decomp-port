/* Owner re-derivation from retail assembly (clean input set). */
/* State handler for a record: runs func_800D1F58 and func_801285D4, clears the byte at +0x15, advances the state byte at +0x14, runs the new state (func_800D1984) and returns 0. */
#include "psx_types.h"

typedef struct Rec800D1938 {
    u8 pad00[0x14];
    u8 state;
    u8 sub;
} Rec800D1938;

extern void func_800D1F58(void);
extern void func_801285D4(void);
extern s32 func_800D1984(Rec800D1938 *r);

s32 func_800D1938(Rec800D1938 *r)
{
    func_800D1F58();
    func_801285D4();
    r->sub = 0;
    r->state = r->state + 1;
    func_800D1984(r);
    return 0;
}
