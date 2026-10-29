/* Owner re-derivation from retail assembly (clean input set). */
/* Looks up an item id for an index, records the index and id, and posts a type-6 request block. */
#include "psx_types.h"

extern u16 D_8006ADD8[];
extern s16 D_800A4E74;
extern s32 D_800A4ECC[];
extern u16 D_800A4ED4;
extern u16 D_800A4ED6;
extern void func_8003B45C(s32 *, s32 *);

void func_8002D240(s16 value) {
    s32 *req;
    u16 item;

    item = D_8006ADD8[value];
    req = D_800A4ECC;
    *req = 6;
    D_800A4E74 = value;
    D_800A4ED4 = item;
    D_800A4ED6 = item;
    func_8003B45C(req, req);
}
