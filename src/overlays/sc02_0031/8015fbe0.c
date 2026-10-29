/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

/* abi void: both direct callers (main_0012 8015FA5C and 8015FBC4) fall
 * straight into their epilogue after the jal with no v0 read; also reached
 * through the D_8018730C-class dispatch table (func_8015C714's jalr never
 * reads v0 either). */

typedef struct {
    u16 f0;
    u8 p2[0x182];
    s32 f184;
} O;

extern void (*D_8018730C[])(O *o);
extern void func_80147078(O *o, s32 a);
extern void func_80159B70(O *o);
extern void func_80154150(O *o, s32 a);
extern void func_80154A74(O *o, s32 a);
extern void func_80146A6C(s32 a, O *o, s32 b, s32 c, s32 d, s32 e, s32 f);
extern void func_80147324(s32 a);
extern void func_801553A8(O *o);
extern void func_801553C0(O *o);
extern void func_80146CA0(O *o);
extern s8 D_80078EC1;
extern s16 D_801270C0;
extern s8 D_800B9A17;

void func_8015FBE0(O *o) {
    if (o->f184 < 0) {
        D_8018730C[o->f0](o);
        func_80147078(o, 0);
        func_80159B70(o);
        return;
    }
    D_80078EC1 = 0;
    func_80154150(o, 0x20);
    func_80154A74(o, 0x18);
    func_80146A6C(0x16, o, 0, 0, 0, 0, 0);
    func_80147324(0x44F);
    func_801553A8(o);
    func_801553C0(o);
    D_801270C0 = 2;
    D_800B9A17 = 0;
    func_80146CA0(o);
}
