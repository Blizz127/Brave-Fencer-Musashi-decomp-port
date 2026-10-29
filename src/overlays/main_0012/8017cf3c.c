/* Owner re-derivation from retail assembly (clean input set). */
/* Selection step entered at func_8017CF3C (loads the mode word D_80185AEC, falls into 8017CF44): handles the open/close request, latches the 8-byte selection state when it changes and dispatches on the new selection. */
#include "psx_types.h"

extern s32 D_80185AEC;
extern s16 D_8011512C;
extern u8 D_80185ADD;
extern u8 D_80185ADC[];
extern u8 D_80185AE4[];
extern u8 D_80185AE5;
extern u8 D_80185AE0[];
extern u8 D_80185AE1;
extern u8 D_80185AE2;
extern u8 D_80185AE3;
extern u32 D_80185AF0[];   /* guest addresses as words (native-lane layout rule) */
extern void *D_80185AF4;
extern void *D_80185AF8;
extern void *D_80185AFC;
extern s32 D_80185AD4;
extern s16 D_80115118;
extern u16 D_8011511A;
extern s16 D_801150D4;
extern s32 D_801826D8;
extern s32 D_80182638;
extern u8 D_801857F8[];

extern void func_8002B00C(void *dst);
extern void func_8017DF40(s16 arg);
extern void func_80029774(s32 index);
extern void func_8002D4C8(u16 command, u16 argument);
extern void *func_8002AF48(s32 i);
extern s32 func_8002AF60(void);
extern void func_8017D730(void);

/* Retail compiled this as an int function that returns no value (v0 stays live at
 * the exit, leaving the early-return delay slots unfilled). Its only caller
 * (8017C2DC) ignores v0. Host builds return an explicit 0. */
#ifdef __mips__
#define RETURN_NO_VALUE return
#else
#define RETURN_NO_VALUE return 0
#endif

s32 func_8017CF3C(void)
{
    s32 mode;
    s32 code;
    s16 i;
    s16 *state;

    mode = D_80185AEC;
    code = 0;
    if (mode != 0) {
        if (mode == 1) {
            state = &D_8011512C;
            if (*state == 3 && D_80185ADD == 8) {
                func_8002B00C(D_801857F8);
                func_8017DF40(0);
                func_80029774(0);
                *state = 4;
                D_80115118 = 0x4B;
                D_801826D8 = 0x15;
                D_801150D4 = D_8011511A;
                func_8002D4C8(0xB3B, 0);
            } else {
                *state = 0;
            }
        } else {
            D_8011512C = 10;
            D_801826D8 = 0x1B;
            D_80115118 = 0x4B;
            D_80185AD4 = 0;
            RETURN_NO_VALUE;
        }
        D_80185AD4 = 0;
        D_80185AE5 = 0xFF;
    }

    if (D_8011512C == 4 || D_8011512C == 10) {
        RETURN_NO_VALUE;
    }
    if (D_80185ADD == D_80185AE5) {
        RETURN_NO_VALUE;
    }
    for (i = 0; i < 8; i++) {
        D_80185AE4[i] = D_80185ADC[i];
    }

    switch (D_80185ADD) {
    case 0:
        break;
    case 1:
        code = 10;
        D_8011512C = 0;
        D_8011511A = 0;
        break;
    case 2:
        code = 11;
        D_801150D4 = 0;
        D_8011512C = 0;
        D_8011511A = 0;
        break;
    case 3:
        code = 0x1F;
        D_801150D4 = 0;
        D_8011512C = 8;
        break;
    case 4:
    case 5:
    case 6:
    retry:
        code = 0x13;
        D_801150D4 = 0;
        D_8011512C = 8;
        break;
    case 7:
        code = 10;
        break;
    case 8:
        code = 0xC;
        if ((D_80185AE0[0] | D_80185AE1 | D_80185AE2 | D_80185AE3) == 0) {
            goto retry;
        }
        for (i = 0; i < 4; i++) {
            if (D_80185AE0[i] == 1) {
                D_80185AF0[i] = (u32)func_8002AF48(i);
            } else {
                D_80185AF0[i] = 0;
            }
        }
        if (D_80185AF0[0] == 0 && D_80185AF4 == 0 && D_80185AF8 == 0 && D_80185AFC == 0) {
            goto retry;
        }
        D_8011512C = 1;
        D_8011511A = (func_8002AF60() & 3) + 1;
        func_8017D730();
        break;
    }
    if (code != 0) {
        D_80182638 = code;
    }
#ifndef __mips__
    return 0;
#endif
}
