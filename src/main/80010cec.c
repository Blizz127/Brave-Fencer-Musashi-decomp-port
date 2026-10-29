/* Owner re-derivation from retail assembly (clean input set). */
/* Runs a three-call sequence when a flag halfword in the game state block is nonzero. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern void func_800CEDFC(void);
extern void func_8001C044(void);
extern void func_80015310(void);
extern void func_80011818(s32);

void func_80010CEC(void) {
    register u8 *base = D_800AF630;

    func_800CEDFC();
    if (*(u16 *)(base + 0xA3B6) != 0) {
        func_8001C044();
        func_80015310();
        func_80011818(3);
    }
}
