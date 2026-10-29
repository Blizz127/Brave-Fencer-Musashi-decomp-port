/* Owner re-derivation from retail assembly (clean input set). */
/* Resets object scale to 1.0, then sets flag 0x200 / 0x400 (scale 1/3) from global state. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ u16 kind;
    /* 0x02 */ u8 pad02[0x42];
    /* 0x44 */ s32 flags;
    /* 0x48 */ u8 pad48[0x20];
    /* 0x68 */ s16 scale[3];
} Obj8014B190;

typedef struct {
    /* 0x00 */ u8 pad00[0x22];
    /* 0x22 */ s16 state;
} Glob8014B190;

extern Glob8014B190 D_80078E78;
extern s16 D_80078E96;
extern u16 D_80078EB8;
extern s16 D_80078EA6;
extern u8 D_80078EC1;

extern s32 func_8016F1C4(void);

void func_8014B190(Obj8014B190 *obj)
{
    Glob8014B190 *g;

    obj->scale[0] = 0x1000;
    obj->scale[1] = 0x1000;
    obj->scale[2] = 0x1000;
    g = &D_80078E78;
    obj->flags &= ~0x200;
    obj->flags &= ~0x400;
    if (func_8016F1C4() != 0) {
        return;
    }
    if (obj->kind == 0x1E || obj->kind == 0x1A) {
        return;
    }
    if ((D_80078E96 & 0x8000) || D_80078EB8 < 11 || D_80078EA6 < 16 || D_80078EC1 == 1) {
        obj->flags |= 0x200;
    }
    if (g->state & 0x8000) {
        obj->scale[0] = 0x555;
        obj->scale[1] = 0x555;
        obj->scale[2] = 0x555;
        obj->flags |= 0x400;
    }
}
