/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u16 id;
    u8 pad[0x66];
} E;

extern u8 D_8011F9D0[];
extern void func_80162CCC(E *e);

void func_8014AB7C(s32 id) {
    E *p;
    for (p = ((E *)&D_8011F9D0); p < &((E *)&D_8011F9D0)[20]; p++) {
        if (p->id == id) {
            func_80162CCC(p);
        }
    }
}
