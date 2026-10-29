/* Owner re-derivation from retail assembly (clean input set). */
/* Runs a wait loop while func_80034B98 reports busy, otherwise tries func_8001AA78; finishes with two common calls. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern s16 D_800B9A04;
extern s32 func_80034B98(void);
extern s32 func_8001AA78(void);
extern void func_8002D4C8(s32, s32);
extern void func_8003500C(void);
extern void func_8002D034(void);
extern void func_8004239C(s32);
extern void func_8002F67C(void);
extern void func_80019990(void);

s32 func_80015144(void) {
    u8 *base = D_800AF630;

    if (func_80034B98()) {
        func_8002D4C8(2, 0);
        do {
            func_8003500C();
            func_8002D034();
            func_8004239C(*(s32 *)(base + 0xA3E8));
        } while (func_80034B98());
        *(s16 *)(base + 0xA3D4) = 1;
    } else {
        if (func_8001AA78()) {
            return 0;
        }
        D_800B9A04 = 1;
        func_8002D4C8(2, 0);
    }
    func_8002F67C();
    func_80019990();
    return 1;
}
