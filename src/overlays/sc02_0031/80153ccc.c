/* Owner re-derivation from retail assembly (clean input set). */
/* func_80019064(&D_80186A68); dispatches to D_80186A90[o->f2] handler table entry; clears
 * D_8011DB0C and D_8011F748. No caller found in any available region. */
#include "psx_types.h"

typedef struct {
    u8 pad0[2];
    u16 f2; /* 0x2 */
} Obj;

typedef void (*Handler)(Obj *);

extern u8 D_80186A68[];
extern void func_80019064(void *a);
extern u32 D_80186A90[];
extern s16 D_8011DB0C;
extern s16 D_8011F748;

void func_80153CCC(Obj *o)
{
    func_80019064(D_80186A68);
    ((Handler)D_80186A90[o->f2])(o);
    D_8011DB0C = 0;
    D_8011F748 = 0;
}
