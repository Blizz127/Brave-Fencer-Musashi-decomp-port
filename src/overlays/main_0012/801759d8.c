/* Owner re-derivation from retail assembly (clean input set). */
/* Unless disabled, splices display list 2 of the current buffer into its OT slot and bumps the slot's word count. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ u32 ot;
    /* 0x04 */ u8 pad04[0x10];
} Buf801759D8; /* 0x14 */

typedef struct {
    /* 0x00 */ s32 words;
    /* 0x04 */ u8 pad04[0x8];
} Cnt801759D8; /* 0x0C */

typedef struct {
    /* 0x00 */ u32 head[2];
    /* 0x08 */ u32 tail[2];
} List801759D8; /* 0x10 */

typedef struct {
    /* 0x00 */ u8 pad00[0x18];
    /* 0x18 */ List801759D8 list[3];
} Ctx801759D8; /* 0x48 */

typedef struct {
    /* 0x00 */ u16 idx;
} Cur801759D8;

extern u8 D_8011F7B1;
extern Cur801759D8 D_800B9A02;
extern Buf801759D8 D_800AE7BC[];
extern Cnt801759D8 D_800AF634[];
extern Ctx801759D8 D_8011F7A8;

void func_801759D8(void)
{
    Ctx801759D8 *c;
    u32 *p;
    u32 *q;
    u32 v;

    c = &D_8011F7A8;
    if (D_8011F7B1 != 0) {
        return;
    }
    p = (u32 *)D_800AE7BC[D_800B9A02.idx].ot;
    v = p[1];
    p[1] = (v & 0xFF000000) | (c->list[2].tail[D_800B9A02.idx] & 0xFFFFFF);
    q = (u32 *)c->list[2].head[D_800B9A02.idx];
    *q = (*q & 0xFF000000) | (v & 0xFFFFFF);
    D_800AF634[D_800B9A02.idx].words +=
        (s32)(c->list[2].tail[D_800B9A02.idx] + 0x14 - c->list[2].head[D_800B9A02.idx]) >> 2;
}
