#include "psx_types.h"

typedef struct {
    u8 b[0x10C];
} E;

extern u8 D_801202A0[];
extern s32 func_8016F1AC(void);
extern s32 func_80149F2C(u8 *o, E *e);

s32 func_80149E94(u8 *o) {
    E *e;
    if (func_8016F1AC() == 0 && o[0x1C5] != 0) {
        o[0x1C5] = 0;
        for (e = ((E *)&D_801202A0); e < &((E *)&D_801202A0)[0x60]; e++) {
            if (func_80149F2C(o, e) != 0) {
                return 1;
            }
        }
    }
    return 0;
}
