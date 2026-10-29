/* Owner re-derivation from retail assembly (clean input set). */
/* Updates the D_80078EBF meter: clear when obj bit0 is off, else add 3 (6 with flag 0x20) while 0xAA bit3 is set, capped at 0x80. */
#include "psx_types.h"

typedef struct Obj8014A59C {
    /* 0x00 */ u8 pad00[0x44];
    /* 0x44 */ u32 flags;
    /* 0x48 */ u8 pad48[0x62];
    /* 0xAA */ u16 state;
} Obj8014A59C;

extern u8 D_80078EBF;

extern u8 func_80029178(s32 flag);

s32 func_8014A59C(Obj8014A59C *obj)
{
    s32 add;

    if (obj->flags & 1) {
        if (obj->state & 8) {
            if (func_80029178(0x20) != 0) {
                add = 6;
            } else {
                add = 3;
            }
            D_80078EBF += add;
            if (D_80078EBF > 0x80) {
                D_80078EBF = 0x80;
            }
        }
    } else {
        D_80078EBF = 0;
    }
}
