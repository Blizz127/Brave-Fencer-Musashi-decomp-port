/* Owner re-derivation from retail assembly (clean input set). */
/* With a zero argument returns func_800D3020(); otherwise returns bit 24 of func_800D30B8(). */
#include "psx_types.h"

extern s32 func_800D3020(void);
extern u32 func_800D30B8(void);

s32 func_800D2CE4(s32 flag) {
    s32 r;

    if (flag == 0) {
        r = func_800D3020();
    } else {
        r = (func_800D30B8() >> 24) & 1;
    }
    return r;
}
