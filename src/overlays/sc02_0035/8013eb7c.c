/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame hand-attack state update: skip entirely unless func_80141C50
 * reports idle; otherwise cache D_801151C8[D_800B9A02], advance the attack
 * phase counter via func_8013FAF8/func_8014168C/func_8014032C, re-trigger
 * func_80139954/func_801376E8 on a phase change, run func_80137B80, and
 * either close the attack (func_8002D4C8 + decrement D_80115112) or run
 * func_8013E958 on the trailing flag checks. */
#include "psx_types.h"

extern s32 func_80141C50(void);
extern s32 func_8013F350(void);
extern void func_8013FAF8(s16 a, s16 b);
extern s16 func_8014168C(s16 a);
extern s32 func_8014032C(s16 a, s16 b);
extern void func_80139954(void);
extern u8 *func_801376E8(u32 a, u16 *p);
extern void func_80137B80(void);
extern void func_8002D4C8(u16 cmd, u16 arg);
extern s32 func_80141CA4(void);
extern void func_8013E958(void);

extern u16 D_800B9A02;
extern u16 D_8011511A;
extern u16 D_8011511E;
extern s16 D_80115128;
extern s16 D_8011512A;
extern s32 D_80115130;
extern s32 D_801151C8[];
extern s32 D_801151D0;
extern u16 D_80115112;
extern u16 D_8018584C;

void func_8013EB7C(void)
{
    s32 s1val;
    s32 s0val;
    s16 a1arg;
    s32 pad[2];
    if ((s16)func_80141C50() == 0) {
        D_801151D0 = D_801151C8[D_800B9A02];
        s1val = func_8013F350();
        if (D_8011511A >= 4) {
            func_8013FAF8((s16)D_8011511A, (s16)(D_8011511A + 1));
        }
        func_8013FAF8(0, 5);
        if (D_8011511A == 5) {
            a1arg = func_8014168C(2);
            s0val = func_8014032C(2, a1arg);
        } else {
            a1arg = func_8014168C((s16)D_8011511A);
            s0val = func_8014032C((s16)D_8011511A, a1arg);
        }
        if (s0val != 0) {
            s32 *p = &D_80115130;
            if (s0val != *p) {
                *p = s0val;
                func_80139954();
                func_801376E8(s0val, &D_8018584C);
            }
        }
        func_80137B80();
        if ((D_8011511E & 0x10) && (D_8011511A < 5) && ((s16)s1val == 0)) {
            D_80115128 = 0;
            func_8002D4C8(0x466, 0);
            if (D_8011511A != 4) {
                func_801376E8(func_80141CA4(), &D_8018584C);
            }
            D_80115112 -= 1;
            return;
        }
        if ((D_8011511E & 0x800) || (D_8011512A != 0)) {
            func_8013E958();
        }
    }
}
