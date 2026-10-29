/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
typedef struct {
    u32 a;
    u32 b;
} Ent80156600;

extern u32 D_801151E0[3];
extern u8 D_80126AF0[];

Ent80156600 *func_80156600(void)
{
    u32 i;

    for (i = 0; i < 3; i++) {
        if ((D_801151E0[i] & 1) != 0) {
            return &((Ent80156600 *)&D_80126AF0)[i];
        }
    }
    return 0;
}
