/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad[0x1F8];
    u32 f1F8;
} Ctx;

extern s32 D_8018175C[];
extern s16 D_800B9A08;
extern Ctx D_80126B58;

s32 func_8016F1C4(void) {
    Ctx *c;
    s32 *p;
    s32 x;

    c = &D_80126B58;
    p = D_8018175C;
    while ((x = *p++) != 0) {
        if (D_800B9A08 == x) {
            return 1;
        }
    }
    if ((c->f1F8 & 0x80FFFFFF) && (c->f1F8 & 0x10000000)) {
        return 0;
    }
    return c->f1F8 & 0x80FFFFFF;
}
