/* Owner re-derivation from retail assembly (clean input set). */
/* Scans the 96-entry object table for an active, flagged entry that passes func_8014C278; hands it to func_801506A4. */
#include "psx_types.h"

typedef struct {
    u16 active00;   /* 0x00 */
    u8 pad02[0x5A];
    u16 flags5C;    /* 0x5C */
    u8 pad5E[0xAE];
} Ent801505FC;      /* size 0x10C */

extern Ent801505FC D_801202A0[];
extern s32 func_8014C278(void *self, Ent801505FC *ent, s32 range);
extern void func_801506A4(void *self, Ent801505FC *ent);

s32 func_801505FC(void *self)
{
    Ent801505FC *ent;

    for (ent = D_801202A0; ent < &D_801202A0[96]; ent++) {
        if (ent->active00 != 0 && (ent->flags5C & 0x100)) {
            if (func_8014C278(self, ent, 0x30)) {
                func_801506A4(self, ent);
                return 1;
            }
        }
    }
    return 0;
}
