/* Owner re-derivation from retail assembly (clean input set). */
/* Cycles a three-step colour pattern into bytes 4-6 of the record and advances the step counter in D_8011F7AB (wraps via the default case). */
#include "psx_types.h"
extern u8 D_8011F7AB;
typedef struct { u8 pad[4]; u8 a, b, c; } O;
void func_801776EC(O *o) {
    u8 *p;
    switch (D_8011F7AB) {
    default:
        D_8011F7AB = 0;
    case 0:
        o->a = 0xFF; o->c = 0x30; o->b = 0x30;
        break;
    case 1:
        o->b = 0xFF; o->c = 0x30; o->a = 0x30;
        break;
    case 2:
        o->c = 0xFF; o->b = 0x30; o->a = 0x30;
        break;
    }
    p = &D_8011F7AB;
    *p = *p + 1;
}
