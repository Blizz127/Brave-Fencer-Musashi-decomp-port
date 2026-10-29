/* Owner re-derivation from retail assembly (clean input set). */
/* Steps a small open/confirm state machine; returns 1 when the sequence finishes. */
#include "psx_types.h"

extern s32 D_800747E4;
extern s32 D_800747E8;
extern s32 D_800747E0;

void func_80036AF8(s32, s16);
s32 func_80034B98(void);
s32 func_8001A114(void);

s32 func_8001B0D4(s32 arg0, s16 arg1) {
    s32 r;

    switch (D_800747E4) {
    case 0:
        func_80036AF8(arg0, arg1);
        D_800747E8 = 1;
        D_800747E4++;
        break;
    case 1:
        r = func_80034B98();
        switch (r) {
        case 0:
            D_800747E0 = 1;
            break;
        case 1:
            D_800747E8 = r;
            break;
        case 8:
            D_800747E8 = 4;
            D_800747E4 = 4;
            break;
        case 2:
        case 4:
            D_800747E8 = r;
            D_800747E4++;
            break;
        }
        break;
    case 2:
        r = func_80034B98();
        switch (r) {
        case 0:
        case 8:
            D_800747E8 = 0;
            return 1;
        case 1:
        case 2:
        case 4:
            D_800747E8 = r;
            break;
        }
        break;
    case 3:
        if (func_8001A114()) {
            D_800747E4 = 0;
        }
        break;
    case 4:
        D_800747E8 = 0;
        return 1;
    }
    return 0;
}
