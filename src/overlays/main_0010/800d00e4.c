/* Owner re-derivation from retail assembly (clean input set). */
/* Starts the sound/script resource selected by the request 1..15 (anything else does nothing): each request passes the
 * address of its own data block to func_8002F5C8. */
#include "psx_types.h"

extern u8 D_800CA23C[];
extern u8 D_800C8448[];
extern u8 D_800CA84C[];
extern u8 D_800CA3E4[];
extern u8 D_800CA344[];
extern u8 D_800C83AC[];
extern u8 D_800C9A30[];
extern u8 D_800C9158[];
extern u8 D_800C9310[];
extern u8 D_800C83A0[];
extern u8 D_800C8058[];
extern u8 D_800CADE8[];
extern u8 D_800C8500[];
extern u8 D_800C8514[];
extern u8 D_800C9140[];
extern s32 func_8002F5C8(u8 *p);

/* Returns retail's leftover v0: the range-test flag (0) when the request is outside 1..15, else func_8002F5C8's result. */
s32 func_800D00E4(s32 kind)
{
    s32 r;
    u32 idx;

    idx = kind - 1;
    r = idx < 15;
    switch (idx) {
    case 0:
        r = func_8002F5C8(D_800CA23C);
        break;
    case 1:
        r = func_8002F5C8(D_800C8448);
        break;
    case 2:
        r = func_8002F5C8(D_800CA84C);
        break;
    case 3:
        r = func_8002F5C8(D_800CA3E4);
        break;
    case 4:
        r = func_8002F5C8(D_800CA344);
        break;
    case 5:
        r = func_8002F5C8(D_800C83AC);
        break;
    case 6:
        r = func_8002F5C8(D_800C9A30);
        break;
    case 7:
        r = func_8002F5C8(D_800C9158);
        break;
    case 8:
        r = func_8002F5C8(D_800C9310);
        break;
    case 9:
        r = func_8002F5C8(D_800C83A0);
        break;
    case 10:
        r = func_8002F5C8(D_800C8058);
        break;
    case 11:
        r = func_8002F5C8(D_800CADE8);
        break;
    case 12:
        r = func_8002F5C8(D_800C8500);
        break;
    case 13:
        r = func_8002F5C8(D_800C8514);
        break;
    case 14:
        r = func_8002F5C8(D_800C9140);
        break;
    }
    return r;
}
