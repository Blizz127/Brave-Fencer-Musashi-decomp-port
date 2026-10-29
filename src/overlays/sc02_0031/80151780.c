/* Owner re-derivation from retail assembly (clean input set). */
/* State-machine step: o->mode selects a handler from the 32-entry table o->table
 * (1-based, table[2] is the out-of-range/zero default). If mode is 0 or >= 0x21, runs
 * table[2](o). Else if table[mode-1] is NULL, runs func_80151878(o). Otherwise runs
 * func_801725CC(), bumps the D_8011DB1A counter via func_80029D3C() unless it is
 * already 1000, re-reads o->mode, runs table[mode-1](o), then D_801151FC = D_80127518.
 * In every case, finally runs func_80153B58(o) when o->f0F0 bit 0x200 is set. Returns
 * void: both direct callers (func_801516F0, func_8015173C) ignore v0, and the sole
 * jalr table referencing this function (the shared per-type handler table, main_0012.s
 * D_80180B24 slot) is never read by its dispatcher either. */
#include "psx_types.h"

typedef struct Obj Obj;
typedef void (*Handler)(Obj *);

struct Obj {
    u8 pad0[0xDD];
    u8 mode;        /* 0xDD */
    u8 padDE[0xF0 - 0xDE];
    u16 f0F0;       /* 0xF0 */
    u8 padF2[0x1F0 - 0xF2];
    Handler *table; /* 0x1F0 */
};

extern s16 D_8011DB1A;
extern u32 D_80127518;
extern u32 D_801151FC;

extern void func_801725CC(void);
extern s32 func_80029D3C(s16 counter);
extern void func_80151878(Obj *o);
extern void func_80153B58(Obj *o);

void func_80151780(Obj *o)
{
    Handler *table;
    u8 mode;
    Handler h;

    table = o->table;
    mode = o->mode;

    if (mode == 0) {
        goto oor;
    }
    if (mode >= 0x21) {
        goto oor;
    }

    h = table[mode - 1];
    if (h == 0) {
        goto nullh;
    }

    func_801725CC();
    if (D_8011DB1A != 1000) {
        D_8011DB1A = func_80029D3C(D_8011DB1A);
    }

    h = table[o->mode - 1];
    h(o);
    D_801151FC = D_80127518;
    goto tail;

nullh:
    func_80151878(o);
    goto tail;

oor:
    h = table[2];
    h(o);

tail:
    if (o->f0F0 & 0x200) {
        func_80153B58(o);
    }
}
