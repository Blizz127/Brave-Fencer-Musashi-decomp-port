/* Owner re-derivation from retail assembly (clean input set). */
/* Splices display list 1 of the current buffer into its OT slot and bumps the slot's word count. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ u32 ot;
    /* 0x04 */ u8 pad04[0x10];
} Buf801758FC; /* 0x14 */

typedef struct {
    /* 0x00 */ s32 words;
    /* 0x04 */ u8 pad04[0x8];
} Cnt801758FC; /* 0x0C */

typedef struct {
    /* 0x00 */ u32 head[2];
    /* 0x08 */ u32 tail[2];
} List801758FC; /* 0x10 */

typedef struct {
    /* 0x00 */ u8 pad00[0x18];
    /* 0x18 */ List801758FC list[3];
} Ctx801758FC; /* 0x48 */

typedef struct {
    /* 0x00 */ u16 idx;
} Cur801758FC;

extern Cur801758FC D_800B9A02;
extern Buf801758FC D_800AE7BC[];
extern Cnt801758FC D_800AF634[];
extern Ctx801758FC D_8011F7A8;

void func_801758FC(void)
{
    Ctx801758FC *c;
    u32 *p;
    u32 *q;
    u32 v;

    p = (u32 *)D_800AE7BC[D_800B9A02.idx].ot;
    v = p[3];
    p[3] = (v & 0xFF000000) | (D_8011F7A8.list[1].tail[D_800B9A02.idx] & 0xFFFFFF);
    q = (u32 *)D_8011F7A8.list[1].head[D_800B9A02.idx];
    *q = (*q & 0xFF000000) | (v & 0xFFFFFF);
    c = &D_8011F7A8;
    D_800AF634[D_800B9A02.idx].words +=
        (s32)(c->list[1].tail[D_800B9A02.idx] + 0x14 - c->list[1].head[D_800B9A02.idx]) >> 2;
}
