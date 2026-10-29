/* Owner re-derivation from retail assembly (clean input set). */
/* Unless D_80115128 is set, run the four scripted hit-reaction calls into
 * func_801412A8 (threshold/base D_80115172+0x31, per-call scale from
 * D_8018532E/32/36/3A), threading the accumulated value arg0 through all
 * four, and return the final accumulator; otherwise return arg0 unchanged. */
#include "psx_types.h"

extern s16 func_8002A1B4(void);
extern s16 func_8002A27C(void);
extern s16 func_8002A28C(void);
extern s16 func_8002A400(void);
extern s16 func_8002A4C8(void);
extern s16 func_8002A4D8(void);
extern s16 func_8002A670(void);
extern s16 func_8002A738(void);
extern s16 func_8002A748(void);
extern s16 func_8002A8E0(void);
extern s16 func_8002A9A8(void);
extern s16 func_8002A9B8(void);
extern s32 func_801412A8(s32 a0, s16 a1, s16 a2, s16 a3, s32 a4, s32 a5);

extern s16 D_80115128;
extern u16 D_80115172;
extern u16 D_8018532E;
extern u16 D_80185332;
extern u16 D_80185336;
extern u16 D_8018533A;

s32 func_80141100(s32 arg0)
{
    s32 acc = arg0;
    s16 base;
    s16 scale;

    if (D_80115128 == 0) {
        base = D_80115172 + 0x31;
        scale = D_8018532E;
        acc = func_801412A8(acc, func_8002A1B4(), func_8002A28C(), func_8002A27C(), base, scale);

        scale = D_80185332;
        acc = func_801412A8(acc, func_8002A400(), func_8002A4D8(), func_8002A4C8(), base, scale);

        scale = D_80185336;
        acc = func_801412A8(acc, func_8002A8E0(), func_8002A9B8(), func_8002A9A8(), base, scale);

        scale = D_8018533A;
        acc = func_801412A8(acc, func_8002A670(), func_8002A748(), func_8002A738(), base, scale);
    }
    return acc;
}
