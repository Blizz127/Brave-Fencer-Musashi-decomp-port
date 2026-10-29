/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 z;
} V;

typedef struct {
    /* 0x00 */ u8 pad00[6];
    /* 0x06 */ u16 x;
    /* 0x08 */ u8 pad08[2];
    /* 0x0A */ u16 y;
    /* 0x0C */ u8 pad0C[2];
    /* 0x0E */ u16 z;
} Obj;

/* Returns a value: the call site at 0x801504A8 stores v0 into D_80184248. */
extern s32 func_80150528(Obj *o, V *a, V *b);

/* Builds a probe pair around o's position (y-3 and y+1) and runs func_80150528 on it.
 * The fields are read into locals in retail load order: x, then y, then z. */
s32 func_801504D8(Obj *o)
{
    V a;
    V b;
    u16 x;
    u16 y;
    u16 z;

    x = o->x;
    b.x = x;
    a.x = x;
    y = o->y;
    a.y = y - 3;
    z = o->z;
    b.y = y + 1;
    b.z = z;
    a.z = z;
    return func_80150528(o, &a, &b);
}
