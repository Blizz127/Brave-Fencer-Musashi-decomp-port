/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame update of the three 0x4C-byte actor slots (focused one first), then the fade slots. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ u32 active;
    /* 0x04 */ s16 kind;
    /* 0x06 */ u8 pad06[2];
    /* 0x08 */ u32 flags;
    /* 0x0C */ u8 pad0C[0x40];
} Obj801379FC; /* size 0x4C */

extern Obj801379FC D_801269F0[];
extern u32 D_80127524; /* guest pointer to the current Obj801379FC (or 0) */
extern s32 D_80127548;

extern void func_80138BE0(Obj801379FC *obj);
extern void func_80137BD8(Obj801379FC *obj);
extern void func_8013A380(void);

void func_801379FC(void)
{
    s32 i;

    D_80127548 = 0x24;
    if (D_80127524 != 0 && ((Obj801379FC *)D_80127524)->active != 0) {
        func_80138BE0((Obj801379FC *)D_80127524);
        for (i = 0; i < 3; i++) {
            if (&D_801269F0[i] != (Obj801379FC *)D_80127524 && D_801269F0[i].active != 0) {
                func_80138BE0(&D_801269F0[i]);
            }
        }
    } else {
        for (i = 0; i < 3; i++) {
            if (D_801269F0[i].active != 0) {
                func_80138BE0(&D_801269F0[i]);
            }
        }
    }
    for (i = 0; i < 3; i++) {
        if (D_801269F0[i].kind != 0 && D_801269F0[i].active != 0
            && (D_801269F0[i].flags & 0x1000) && !(D_801269F0[i].flags & 0x2000)) {
            func_80137BD8(&D_801269F0[i]);
        }
    }
    func_8013A380();
}
