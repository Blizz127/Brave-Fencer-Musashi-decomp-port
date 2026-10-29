/* Owner re-derivation from retail assembly (clean input set). */
/* Walks the D_801274CC entry list; for each unflagged entry in range, binds a free object slot via func_8012C724. */
#include "psx_types.h"

typedef struct Ent8013E370 {
    /* 0x00 */ s16 pos[3];
    /* 0x06 */ u16 kind;
    /* 0x08 */ u8 pad08[2];
    /* 0x0A */ u16 flags;
    /* 0x0C */ u8 pad0C[8];
} Ent8013E370; /* size 0x14 */

extern u32 D_801274CC; /* guest pointer: entry list */

extern void *func_8013E410(void);
extern s32 func_8013E448(s16 *pos);
extern void func_8012C724(Ent8013E370 *ent, void *slot);

void func_8013E370(void)
{
    Ent8013E370 *e;
    void *slot;

    e = (Ent8013E370 *)D_801274CC;
    if (e == 0) {
        return;
    }
    for (; e->kind != 0; e++) {
        if (e->flags & 0x8000) {
            continue;
        }
        slot = func_8013E410();
        if (slot == 0) {
            return;
        }
        if (func_8013E448(e->pos)) {
            func_8012C724(e, slot);
        }
    }
}
