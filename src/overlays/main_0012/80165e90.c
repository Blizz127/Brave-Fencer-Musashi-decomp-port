/* Owner re-derivation from retail assembly (clean input set). */
/* Runs the two state-gated update passes on the shared controller block when its target is active. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ s16 x;
    /* 0x02 */ s16 y;
    /* 0x04 */ s16 z;
    /* 0x06 */ s16 pad;
} Vec80165E90; /* 0x08 */

typedef struct {
    /* 0x00 */ u8 pad00[0x4];
    /* 0x04 */ s32 state;
    /* 0x08 */ u8 pad08[0x18];
    /* 0x20 */ s32 active;
} Target80165E90;

typedef struct {
    /* 0x00 */ u8 pad00[0x20];
    /* 0x20 */ u32 target;
    /* 0x24 */ u8 pad24[0x20];
    /* 0x44 */ u32 flags;
} Ctl80165E90;

extern Ctl80165E90 D_80126B58;

extern void func_801484B0(Vec80165E90 *a, Vec80165E90 *b);
extern void func_80148534(Vec80165E90 *a, Vec80165E90 *b);
extern void func_80165FA4(Ctl80165E90 *ctl, Vec80165E90 *a, Vec80165E90 *b);
extern void func_80167DBC(Ctl80165E90 *ctl, Vec80165E90 *a, Vec80165E90 *b);
extern void func_80167AE0(Ctl80165E90 *ctl, Vec80165E90 *a, Vec80165E90 *b);

void func_80165E90(void)
{
    Vec80165E90 a;
    Vec80165E90 b;
    Ctl80165E90 *ctl;
    Target80165E90 *t;

    ctl = &D_80126B58;
    if ((D_80126B58.flags & 0x10000400) == 0x10000000) {
        t = (Target80165E90 *)D_80126B58.target;
        if (t->state >= 0 && t->active != 0) {
            func_801484B0(&a, &b);
            func_80165FA4(ctl, &a, &b);
            func_80167DBC(ctl, &a, &b);
        }
    }
    if ((ctl->flags & 0x20020400) == 0x20000000) {
        t = (Target80165E90 *)ctl->target;
        if (t->state >= 0 && t->active != 0) {
            func_80148534(&a, &b);
            func_80165FA4(ctl, &a, &b);
            func_80167AE0(ctl, &a, &b);
        }
    }
}
