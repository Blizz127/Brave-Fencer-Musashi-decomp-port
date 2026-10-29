/* Owner re-derivation from retail assembly (clean input set). */
/* Converts a number into a 0xFFFF-terminated string of digit glyphs `digits` wide, blank-padding leading zeros (the last digit always shown). */
#include "psx_types.h"

extern s16 D_800636D8[];
extern s16 D_80063818[];
extern const s16 D_80063870[];

void func_800241C0(s16 value, s16 *out, s32 digits) {
    s32 unused_frame[1]; /* retail reserves an unused stack slot */
    s16 i;
    s16 digit;
    s32 shown;

    shown = 0;
    for (i = digits - 1; i >= 0; i--, out++) {
        if (i != 0) {
            *out = D_80063818[0];
        } else {
            *out = D_800636D8[0];
        }
        digit = value / D_80063870[i];
        if (digit != 0) {
            shown++;
            *out = D_800636D8[digit];
            value -= digit * D_80063870[i];
        } else if (shown != 0) {
            *out = D_800636D8[0];
        }
    }
    *out = -1;
}
