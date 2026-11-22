/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the first live, flagged object in D_801202A0 within range 0x30 of obj, else 0. */
#include "scene_object_probes.h"

Obj8014A2E4 *func_8014A2E4(void *obj)
{
    u32 i;
    Obj8014A2E4 *p;

    i = 0;
    p = D_801202A0;
loop:
    if (p->active != 0 && (p->flags & 0x8000) && p->hp != 0) {
        if (func_8014C278((char *)obj, (char *)p, 0x30)) {
            return p;
        }
    }
    i++;
    p++;
    if (i < 0x60) {
        goto loop;
    }
    return 0;
}
