/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 f0;
    u8 f1;
    u8 f2;
    u8 f3;
    u8 _4[28];
} Ent;
extern u8 D_80115218[];

void func_8016E918(s16 a, s16 b) {
    if (b < 10) {
        Ent *e = &((Ent *)&D_80115218)[a];
        e->f0 = a + 1;
        e->f3 = b;
        e->f1 = 0;
    }
}
