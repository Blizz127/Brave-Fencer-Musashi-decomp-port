/* Owner re-derivation from retail assembly (clean input set). */
/* Runs an overlay hook, then schedules task 10 if a status halfword is nonzero. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern void func_800CEE74(void);
extern void func_80011818(s32);

void func_800111BC(void) {
    register u8 *base = D_800AF630;
    func_800CEE74();
    if (*(u16 *)(base + 0xA3B6) != 0) {
        func_80011818(10);
    }
}
