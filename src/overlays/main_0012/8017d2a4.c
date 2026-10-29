/* Owner re-derivation from retail assembly (clean input set). */
/* Selection step entered at func_8017D2A4 (loads the mode word D_80185AEC, falls into 8017D2AC): handles the open/close request, latches the 8-byte selection state when it changes and dispatches on the new selection. */
#include "psx_types.h"

extern s32 D_80185AEC;
extern s16 D_8011512C;
extern u8 D_80185ADD;
extern u8 D_80185ADC[];
extern u8 D_80185AE4[];
extern u8 D_80185AE5;
extern u8 D_80185AE0[];
extern u32 D_80185AF0[];   /* guest addresses as words (native-lane layout rule) */
extern void *D_80185AF4;
extern void *D_80185AF8;
extern void *D_80185AFC;
extern s32 D_80185AD4;
extern s16 D_80115118;
extern u16 D_8011511A;
extern u16 D_801150D4;
extern s32 D_801826D8;
extern s32 D_80182638;

extern void func_80029774(s32 index);
extern void func_8002D4C8(u16 command, u16 argument);
extern void *func_8002AF48(s32 i);
extern s32 func_8002AF60(void);
extern void func_8017D730(void);

/* Retail compiled this as an int function that returns no value (v0 stays live at
 * the exit). Its only caller (8017C2EC) ignores v0. Host builds return an explicit 0. */
#ifdef __mips__
#define RETURN_NO_VALUE return
#else
#define RETURN_NO_VALUE return 0
#endif

s32 func_8017D2A4(void)
{
    s32 mode;
    s32 code;
    s16 i;
    s16 *sel;

    mode = D_80185AEC;
    code = 0;
    if (mode != 0) {
        if (mode == 1) {
            s16 *state = &D_8011512C;

            if (*state == 3) {
                if (D_80185ADD == 8) {
                    func_80029774(0);
                    *state = 4;
                    D_80115118 = 0x4B;
                    D_801150D4 = 0;
                    D_801826D8 = 0xF;
                    func_8002D4C8(0xB3B, 0);
                } else {
                    *state = 0;
                }
            } else if (D_80185ADD == 6) {
                *state = 7;
                D_80115118 = 0x4B;
                D_801826D8 = 0x18;
            } else {
                *state = 0;
            }
        } else if ((D_80185AD4 & 3) == 1) {
            D_8011512C = 11;
            D_801826D8 = 0x12;
            D_80115118 = 0x4B;
        } else {
            D_8011512C = 12;
            D_801826D8 = 0x1C;
            D_80115118 = 0x4B;
        }
        D_80185AD4 = 0;
        D_80185AE5 = 0xFF;
    }

    if (D_8011512C == 11 || D_8011512C == 12) {
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
    case 3: {
        s32 cur;

        sel = &D_8011512C;
        cur = *sel;
        D_801150D4 = 0;
        if (cur != 12) {
            code = 0x11;
            *sel = 5;
        }
        break;
    }
    case 4:
        D_801150D4 = 0;
        D_8011512C = 8;
        code = 0xB;
        break;
    case 5:
        D_8011512C = 8;
        code = 0x10;
        break;
    case 6: {
        s16 *state = &D_8011512C;

        D_801150D4 = 0;
        if (*state != 6 && *state != 7 && *state != 11) {
            code = 0xD;
            *state = 1;
        }
        for (i = 0; i < 4; i++) {
            D_80185AF0[i] = 0;
        }
        D_8011511A = 1;
        func_8017D730();
        break;
    }
    case 7:
        code = 10;
        break;
    case 8: {
        if (D_8011512C != 11 && D_8011512C != 3 && D_8011512C != 4) {
            code = 0xD;
        }
        for (i = 0; i < 4; i++) {
            if (D_80185AE0[i] == 1) {
                D_80185AF0[i] = (u32)func_8002AF48(i);
            } else {
                D_80185AF0[i] = 0;
            }
        }
        sel = &D_8011512C;
        if (*sel == 0) {
            u32 last;

            last = D_801150D4;
            *sel = 1;
            if (last - 1 < 4) {
                D_8011511A = last;
            } else {
                D_8011511A = (func_8002AF60() & 3) + 1;
            }
            if (D_80185AF0[0] == 0 && D_80185AF4 == 0 && D_80185AF8 == 0 && D_80185AFC == 0) {
                D_801150D4 = 0;
                D_8011511A = 1;
            }
        }
        func_8017D730();
        break;
    }
    }
    if (code != 0) {
        D_80182638 = code;
    }
#ifndef __mips__
    return 0;
#endif
}
