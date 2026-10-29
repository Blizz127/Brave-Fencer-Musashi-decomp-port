/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct { u32 f0; } Entry;

extern u16 D_800B9A02;
extern Entry D_801151C8[];
extern u32 D_801151D0;
extern u16 D_8011511A; /* read via lhu; (s16) cast is the retail sign-extend */
extern u8 D_8011514D;
extern u8 D_8011515C;
extern u8 D_8011514C;
extern s32 D_80115130;
extern u8 D_8018584C[];
extern u16 D_80115112;
extern u16 D_80115116;

extern void func_8013F350(void);
extern s32 func_8013FAF8(s32 a0, s32 a1);
extern s32 func_8014032C(s32 a0, s32 a1);
extern void func_80139954(void);
extern s32 func_801376E8(s32 id, u8 *p);
extern void func_80137B80(void);
extern s32 func_800D0EC4(void);

void func_80141A60(void) {
    u16 idx;
    u16 v1;
    s32 prod;
    s16 b2;
    s32 result;
    s32 s0;
    s32 pad[2]; /* retail frame reserves 8 extra bytes of locals that no path touches */

    idx = D_800B9A02;
    D_801151D0 = D_801151C8[idx].f0;
    func_8013F350();

    v1 = D_8011511A;
    if (v1 < 4) {
        func_8013FAF8(2, 3);
    } else {
        func_8013FAF8((s16)v1, (s16)(v1 + 1));
        func_8013FAF8(2, 3);
    }

    prod = D_8011514D * D_8011515C;
    b2 = D_8011514C + prod;
    result = func_8014032C(2, b2);
    s0 = result;
    if (s0 != 0 && s0 != D_80115130) {
        D_80115130 = s0;
        func_80139954();
        func_801376E8(s0, D_8018584C);
    }

    func_80137B80();
    if (func_800D0EC4() != 0) {
        u16 cnt;

        cnt = D_80115112;
        D_80115116 = 0xFFF8;
        cnt = cnt + 1;
        D_80115112 = cnt;
    }
}
