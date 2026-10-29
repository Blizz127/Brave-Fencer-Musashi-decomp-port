/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u16 id;
    u8 pad[0x56];
} E;

extern u8 D_8011D030[];
extern s32 func_80146994(s32 a, s32 b, s32 c, s32 d);

s32 func_80146924(u16 a, s32 b, s32 c, s32 d, u32 lim) {
    u32 n;
    E *p;
    E *end;
    n = 0;
    p = ((E *)&D_8011D030);
    end = p + 30;
    for (; p < end; p++) {
        if (p->id == a) {
            n++;
        }
    }
    if (n < lim) {
        return func_80146994(a, b, c, d);
    }
    return 0;
}