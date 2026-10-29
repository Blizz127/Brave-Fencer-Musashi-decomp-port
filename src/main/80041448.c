/* Owner re-derivation from retail assembly (clean input set). */
/* Triggers func_80040868 for every active record whose id matches, selecting each one in turn. */
#include "psx_types.h"

typedef struct {
    s16 id;
    u8 pad2[0x34 - 2];
} Record;

extern void func_80040868(s32);
extern Record D_80079596[];
extern u8 D_800AE614;
extern s16 D_800B9B3A;

void func_80041448(s16 id) {
    u8 i;

    for (i = 0; i < D_800AE614; i++) {
        if (D_80079596[i].id == id) {
            D_800B9B3A = i;
            func_80040868(0);
        }
    }
}
