/* Owner re-derivation from retail assembly (clean input set). */
/* Dispatches a stream/sequence control request by mode and records the request parameters. */
#include "psx_types.h"

extern s16 D_80063138[];
extern s16 D_8006313C[];
extern void func_8002D4C8(s32, s32);
extern s32 D_800747E4;
extern s32 D_800747E8;
extern s32 D_800747F0;
extern s32 D_800747F4;
extern s32 D_800747F8;
extern s32 D_800A6430;
extern s32 D_800A654C;
extern s32 D_800A6550;
extern s32 D_800AE640;
extern u32 D_800AE6E4; /* guest s32 * */
extern s32 D_800BA1B4;
extern s32 D_800BA318;
extern s32 D_800C6D2C;

s32 func_8001ACF0(s32 mode, s32 arg1, s32 *data, s32 arg3, s32 arg4)
{
    s16 id;

    D_800747F0 = mode;
    switch (mode) {
    case 0:
        D_800C6D2C = arg1;
        D_800AE6E4 = (u32)data;
        D_800BA318 = arg3;
        D_800A6550 = arg4;
        D_800747F4 = *data;
        D_800BA1B4 = 1;
        break;
    case 1:
        D_800BA1B4 = 2;
        if (arg1 == D_800A654C) {
            return 1;
        }
        if (D_80063138[arg1 * 3] == D_800747F8) {
            id = D_8006313C[arg1 * 3];
            if (id != 0) {
                func_8002D4C8((u16)id, 0);
                D_800A654C = arg1;
            }
            return 1;
        }
        D_800AE640 = arg1;
        D_800747F4 = arg1;
        break;
    case 2:
        D_800AE6E4 = (u32)data;
        D_800747F4 = *data;
        D_800BA1B4 = 4;
        break;
    case 3:
        D_800A6430 = arg1;
        D_800AE6E4 = (u32)data;
        D_800747F4 = arg1;
        D_800BA1B4 = 5;
        D_800747E4 = 0;
        D_800747E8 = 1;
        break;
    }
    return 0;
}
