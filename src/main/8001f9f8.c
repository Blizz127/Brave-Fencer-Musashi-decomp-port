/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame model draw pass: submits seven fixed object pools to func_8001FB8C, skipping the pools whose bit is set in the draw mask. */
#include "psx_types.h"

typedef struct {
    u8 pad[0x84];
} Obj8001F9F8;

extern u8 D_800B9A11;
extern Obj8001F9F8 D_800AF8D8[];
extern Obj8001F9F8 D_800AFB6C[];
extern Obj8001F9F8 D_800AFBF0[];
extern Obj8001F9F8 D_800B0430[];
extern Obj8001F9F8 D_800B0C70[];
extern Obj8001F9F8 D_800B3DF0[];
extern void func_8001FB8C(Obj8001F9F8 *obj, s32 *count);

void func_8001F9F8(void)
{
    s32 count;
    s32 i;
    Obj8001F9F8 *p;
    u32 mask;

    mask = D_800B9A11;
    count = 0;
    for (i = 0, p = D_800AF8D8; i < 4; i++, p++) {
        func_8001FB8C(p, &count);
    }
    if (!(mask & 2)) {
        for (i = 0, p = &D_800AF8D8[4]; i < 1; i++, p++) {
            func_8001FB8C(p, &count);
        }
    }
    if (!(mask & 8)) {
        for (i = 0, p = D_800AFB6C; i < 1; i++, p++) {
            func_8001FB8C(p, &count);
        }
    }
    if (!(mask & 0x10)) {
        for (i = 0, p = D_800AFBF0; i < 16; i++, p++) {
            func_8001FB8C(p, &count);
        }
    }
    if (!(mask & 0x20)) {
        for (i = 0, p = D_800B0430; i < 16; i++, p++) {
            func_8001FB8C(p, &count);
        }
    }
    if (!(mask & 0x40)) {
        for (i = 0, p = D_800B0C70; i < 96; i++, p++) {
            func_8001FB8C(p, &count);
        }
    }
    if (!(mask & 0x80)) {
        for (i = 0, p = D_800B3DF0; i < 58; i++, p++) {
            func_8001FB8C(p, &count);
        }
    }
}
