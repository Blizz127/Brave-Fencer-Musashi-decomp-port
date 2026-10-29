/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 state;
    u8 pad01[3];
} Ctrl801627E8;

extern Ctrl801627E8 D_8011F750;
extern u32 D_80180F24[]; /* guest handler table */

void func_801627E8(void) {
    Ctrl801627E8 *c;
    s32 k;

    c = &D_8011F750;
    if (c->state != 0) {
        k = c->state - 1;
        ((void (*)(Ctrl801627E8 *))D_80180F24[k])(c);
    }
}
