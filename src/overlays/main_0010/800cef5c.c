/* Owner re-derivation from retail assembly (clean input set). */
/* Resets the main-loop state: runs func_8001B324, sets the byte flag D_800B9A12 and clears D_800B9A10, D_800B9A17,
 * runs func_8001C0C8, clears the half-word D_800B9A0E and starts func_80011A3C. */
#include "psx_types.h"

extern u8 D_800B9A12;
extern u8 D_800B9A10;
extern u8 D_800B9A17;
extern u16 D_800B9A0E;
extern void func_8001B324(void);
extern void func_8001C0C8(void);
extern s32 func_80011A3C(void);

/* Returns retail's leftover v0: func_80011A3C's result. */
s32 func_800CEF5C(void)
{
    u8 pad[8]; /* unused frame space kept to match the retail frame size */

    func_8001B324();
    D_800B9A12 = 1;
    D_800B9A10 = 0;
    D_800B9A17 = 0;
    func_8001C0C8();
    D_800B9A0E = 0;
    return func_80011A3C();
}
