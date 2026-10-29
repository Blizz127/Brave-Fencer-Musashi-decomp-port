/* Owner re-derivation from retail assembly (clean input set). */
/* Steps each of 9 byte counters (D_801BE8F0..D_801BE8F8) by 8 toward its matching target
 * (D_801BE990..D_801BE998), unconditionally (no snap-when-close clamp). */
#include "psx_types.h"

extern u8 D_801BE8F0;
extern u8 D_801BE8F1;
extern u8 D_801BE8F2;
extern u8 D_801BE8F3;
extern u8 D_801BE8F4;
extern u8 D_801BE8F5;
extern u8 D_801BE8F6;
extern u8 D_801BE8F7;
extern u8 D_801BE8F8;
extern u8 D_801BE990;
extern u8 D_801BE991;
extern u8 D_801BE992;
extern u8 D_801BE993;
extern u8 D_801BE994;
extern u8 D_801BE995;
extern u8 D_801BE996;
extern u8 D_801BE997;
extern u8 D_801BE998;

void func_8013D178(void)
{
    u8 cur;

    {
        u8 *p = &D_801BE8F0;
        cur = *p;
        if (cur != D_801BE990) {
            if (cur < D_801BE990) {
                *p = cur + 8;
            } else {
                *p = cur - 8;
            }
        }
    }

    {
        u8 *p = &D_801BE8F1;
        cur = *p;
        if (cur != D_801BE991) {
            if (cur < D_801BE991) {
                *p = cur + 8;
            } else {
                *p = cur - 8;
            }
        }
    }

    {
        u8 *p = &D_801BE8F2;
        cur = *p;
        if (cur != D_801BE992) {
            if (cur < D_801BE992) {
                *p = cur + 8;
            } else {
                *p = cur - 8;
            }
        }
    }

    {
        u8 *p = &D_801BE8F3;
        cur = *p;
        if (cur != D_801BE993) {
            if (cur < D_801BE993) {
                *p = cur + 8;
            } else {
                *p = cur - 8;
            }
        }
    }

    {
        u8 *p = &D_801BE8F4;
        cur = *p;
        if (cur != D_801BE994) {
            if (cur < D_801BE994) {
                *p = cur + 8;
            } else {
                *p = cur - 8;
            }
        }
    }

    {
        u8 *p = &D_801BE8F5;
        cur = *p;
        if (cur != D_801BE995) {
            if (cur < D_801BE995) {
                *p = cur + 8;
            } else {
                *p = cur - 8;
            }
        }
    }

    {
        u8 *p = &D_801BE8F6;
        cur = *p;
        if (cur != D_801BE996) {
            if (cur < D_801BE996) {
                *p = cur + 8;
            } else {
                *p = cur - 8;
            }
        }
    }

    {
        u8 *p = &D_801BE8F7;
        cur = *p;
        if (cur != D_801BE997) {
            if (cur < D_801BE997) {
                *p = cur + 8;
            } else {
                *p = cur - 8;
            }
        }
    }

    {
        u8 *p = &D_801BE8F8;
        cur = *p;
        if (cur != D_801BE998) {
            if (cur < D_801BE998) {
                *p = cur + 8;
            } else {
                *p = cur - 8;
            }
        }
    }
}
