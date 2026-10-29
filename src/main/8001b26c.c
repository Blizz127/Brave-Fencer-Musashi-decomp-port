/* Owner re-derivation from retail assembly (clean input set). */
/* Arms request block D_800C7C60 (mode 8) and records whether the pending request completed (unset when busy, as retail). */
#include "psx_types.h"

typedef struct {
    s32 mode;
} ReqBlock;

extern u8 D_800A2E20[];
extern u8 D_800AEC78[];
extern s32 D_800BA1B4;
extern s32 D_800C6D10;
extern s32 D_800C6D34;
extern ReqBlock D_800C7C60;
extern u32 D_800C7C64; /* guest u8 * */

s32 func_8001ACF0(s32, s32, u8 *, s32, s32 *);
s32 func_8001AE90(s32, s32, u8 *);

void func_8001B26C(void)
{
    s32 result;
    ReqBlock *req;

    if (D_800C6D34 == 0) {
        req = &D_800C7C60;
        req->mode = 8;
        D_800C7C64 = (u32)D_800A2E20;
        if (D_800BA1B4 != 0) {
            if (D_800BA1B4 != 3) {
                result = 0;
                goto end;
            }
            if (func_8001AE90(0, 0, D_800AEC78) != 0) {
                goto done;
            }
        }
        if (func_8001ACF0(0, 0, D_800AEC78, 0, &req->mode) == 0) {
            result = 0;
            goto end;
        }
done:
        D_800BA1B4 = 0;
        result = 1;
    }
end:
    D_800C6D10 = result;
}
