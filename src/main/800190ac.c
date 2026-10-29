/* Owner re-derivation from retail assembly (clean input set). */
/* Steps the controller vibration script: each 3-byte entry sets the two motor levels for a number of frames; a zero duration ends the script. */
#include "psx_types.h"

typedef struct {
    u8 large;
    u8 small;
    u8 frames;
} RumbleStep;

extern RumbleStep *D_800747C0;
extern u8 D_800747C4;
extern u8 D_800747C5;
extern u8 D_80078DE2;
extern u8 D_80078DE3;

void func_800190AC(void) {
    RumbleStep **script;

    script = &D_800747C0;
    if (*script != 0) {
        if (D_800747C5 == 0) {
            D_800747C4++;
            D_80078DE2 = D_800747C0[D_800747C4].small;
            D_80078DE3 = D_800747C0[D_800747C4].large;
            D_800747C5 = D_800747C0[D_800747C4].frames;
            if (D_800747C5 == 0) {
                D_80078DE2 = 0;
                D_80078DE3 = 0;
                D_800747C4 = 0xFF;
                *script = 0;
            }
        } else {
            D_80078DE2 = D_800747C0[D_800747C4].small;
            D_80078DE3 = D_800747C0[D_800747C4].large;
            D_800747C5--;
        }
    }
}
