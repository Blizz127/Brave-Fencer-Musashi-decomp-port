/* func_8013EA54 [8013EA54,8013EB7C) - owner re-derivation from retail assembly (clean input set).
 * Menu input step: copies D_801151C8[page] to D_801151D0, runs the cursor handler, then handles the 0x40 and 0x810 buttons. */
#include "psx_types.h"

/* callers read v0: none-direct; void. */
extern s16 func_80141C50(void);
extern void func_8013F244(void);
extern void func_8013FAF8(s32 a, s32 b);
extern void func_80137B80(void);
extern void func_8002D4C8(s32 a, s32 b);
extern void func_8013E958(void);
extern u16 D_800B9A02;
extern u32 D_801151C8[];
extern u32 D_801151D0;
extern u16 D_8011511E;
extern u16 D_8011511A;
extern s16 D_800B9A08;
extern s32 D_80115130;
extern u16 D_8011514C;
extern u16 D_80115112;

void func_8013EA54(void) {
    s32 pad[2]; /* unreferenced: retail frame keeps 8 bytes below ra */
    u16 t;
    u16 a;

    if (func_80141C50() != 0) {
        return;
    }
    D_801151D0 = D_801151C8[D_800B9A02];
    func_8013F244();
    func_8013FAF8(0, 5);
    func_80137B80();
    t = D_8011511E;
    if (t & 0x40) {
        if (D_8011511A == 0 && D_800B9A08 == 0x308F) {
            func_8002D4C8(0x466, 0);
            return;
        }
        a = D_8011511A;
        if (a != 4) {
            D_80115130 = 0;
        }
        if (a == 1 || a == 2) {
            D_8011514C = 0;
        }
        func_8002D4C8(0x45D, 0);
        D_80115112 = D_80115112 + 1;
    } else if (t & 0x810) {
        func_8013E958();
    }
}
