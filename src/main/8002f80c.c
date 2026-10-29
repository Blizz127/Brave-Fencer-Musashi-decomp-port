/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame poll of four request bits: forwards the largest level, or runs the release path when the hold count expires. */
#include "psx_types.h"

extern u8 D_8006A96E;
extern s16 D_8006A96C;
extern u8 D_800760D0;
extern u8 D_800760D4;
extern u8 D_800760D8;
extern u8 D_800760DC;
extern u8 D_800A4F17;
extern u8 D_800A4F18;
extern s16 D_800A4EFC;
extern s32 D_800A4F24;

void func_8002DC68(s32, s32);
void func_8002E138(s32, s32, s32);
void func_80030F80(void);
void func_8002D320(void);
void func_80031BE0(void);

void func_8002F80C(void) {
    u8 hit;
    s32 vol;
    s32 t;
    u8 flags;

    hit = 0;
    vol = 0;
    if (D_8006A96E & 1) {
        hit = 1;
        vol = D_800760D0;
    }
    if (D_8006A96E & 2) {
        hit = 1;
        t = D_800760D4;
        if (vol < t) {
            vol = (u8)t;
        }
    }
    if (D_8006A96E & 4) {
        hit = 1;
        t = D_800760D8;
        if (vol < t) {
            vol = (u8)t;
        }
    }
    if (D_8006A96E & 8) {
        hit = 1;
        t = D_800760DC;
        if (vol < t) {
            vol = (u8)t;
        }
    }
    flags = D_8006A96E;
    D_8006A96E = flags & 0xF0;
    if (hit) {
        D_8006A96C = 0x10;
        if (vol != 0) {
            func_8002DC68(0xBEB, vol | 0x1000);
            D_8006A96E |= 0x10;
        } else if (flags & 0x10) {
            D_800A4F17 = 1;
            func_8002E138(4, 0xBEB, 0);
            D_800A4F17 = D_800A4F18;
            if (D_800A4F18 != 0) {
                if (D_800A4EFC != 0) {
                    D_800A4EFC--;
                }
                func_80030F80();
                func_8002D320();
                func_80031BE0();
                D_800A4F18 = 0;
                D_800A4F17 = 0;
            }
            D_800A4F24 = 0;
            D_8006A96E &= 0xEF;
        }
    } else if (D_8006A96C != 0) {
        if (--D_8006A96C == 0 && (flags & 0x10)) {
            D_800A4F17 = 1;
            func_8002E138(4, 0xBEB, 0);
            D_800A4F17 = D_800A4F18;
            if (D_800A4F18 != 0) {
                if (D_800A4EFC != 0) {
                    D_800A4EFC--;
                }
                func_80030F80();
                func_8002D320();
                func_80031BE0();
                D_800A4F18 = 0;
                D_800A4F17 = 0;
            }
            D_800A4F24 = 0;
            D_8006A96E &= 0xEF;
        }
    }
}
