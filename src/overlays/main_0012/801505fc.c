/* Owner re-derivation from retail assembly (clean input set). */
/* Scans the 96-entry object table for an active, flagged entry that passes func_8014C278; hands it to func_801506A4. */
#include "scene_object_probes.h"

extern void func_801506A4(void *self, Obj8014A2E4 *ent);

s32 func_801505FC(void *self)
{
    Obj8014A2E4 *ent;

    for (ent = D_801202A0; ent < &D_801202A0[96]; ent++) {
        if (ent->active != 0 && (ent->flags & 0x100)) {
            if (func_8014C278((char *)self, (char *)ent, 0x30)) {
                func_801506A4(self, ent);
                return 1;
            }
        }
    }
    return 0;
}
