/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

/* Dispatch-table handler (D_80180BA8-class tables via func_8015C714's jalr);
 * confirmed void: that dispatcher never reads v0 after the call. */

typedef struct {
    u16 f0;
    u8 p2[0xB6];
    u16 fB8;
    u8 pBA[0xB4];
    u16 f16E;
    s32 f170;
} O;

extern void func_80149FB0(void);
extern void func_80147AD4(O *o, s32 a, s32 b, s32 c);
extern void func_801473EC(O *o);
extern void func_8014D738(O *o);
extern void func_80147078(O *o, s32 a);
extern void func_80159B70(O *o);
extern s32 func_80161240(O *o);
extern s32 func_80146E98(O *o);
extern void func_80146CA0(O *o);
extern void func_80161C24(O *o, u16 a);
extern void (*D_8018730C[])(O *o);

void func_8015C944(O *o) {
    func_80149FB0();
    func_80147AD4(o, 0, 0, 0);
    func_801473EC(o);
    func_8014D738(o);
    if (o->f170 == 0) {
        D_8018730C[o->f0](o);
        func_80147078(o, 0);
        func_80159B70(o);
        return;
    }
    if (func_80161240(o) == 0) {
        if (func_80146E98(o) != 0) {
            func_80146CA0(o);
        }
        if (o->fB8 & 0x2000) {
            func_80161C24(o, o->f16E);
        }
    }
}
