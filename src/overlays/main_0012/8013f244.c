/* func_8013F244 [8013F244,8013F348) - owner re-derivation from retail assembly (clean input set).
 * Menu cursor D_8011511A (0..3) moved by button bits in D_8011511E; calls func_80141C0C(2) when it changed. */
#include "psx_types.h"

/* callers read v0: none-direct; void. D_8011511A is accessed through a pointer (pa) in the first
 * block and the else arm, and as the plain scalar in the other two blocks (retail mixes both forms). */
extern void func_80141C0C(s32 a);
extern u16 D_8011511A;
extern u16 D_8011511E;

void func_8013F244(void) {
    u16 *pa;
    s16 n;

    pa = &D_8011511A;
    n = 0;
    if (*pa < 3) {
        if (D_8011511E & 0x2000) {
            *pa = *pa + 1;
            n = 1;
            if (*pa >= 3) {
                *pa = 0;
            }
        }
        if (D_8011511E & 0x8000) {
            D_8011511A = D_8011511A - 1;
            if (D_8011511A >= 3) {
                D_8011511A = 2;
            }
            n++;
        }
        if (D_8011511E & 0x5000) {
            D_8011511A = 3;
            n++;
        }
    } else {
        if (D_8011511E & 0x5000) {
            *pa = 0;
            n = 1;
        }
    }
    if (n != 0) {
        func_80141C0C(2);
    }
}
