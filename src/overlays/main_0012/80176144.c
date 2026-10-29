/* func_80176144 [80176144,80176218) - owner re-derivation from retail assembly (clean input set). */
/* Closes display list i: stores the end of its counter-table words at ctx+0x38+i*4, lets
 * func_80178004 emit the footer there, records end-0x14 at +0x40 and adds the emitted words. */
#include "psx_types.h"

/* view of the tables as in func_80175820 (owner): counter table D_800AF634[] (12-byte records) and the
 * display-list context D_8011F7A8. */
typedef struct {
    /* 0x00 */ s32 words;
    /* 0x04 */ u32 base;
    /* 0x08 */ u8 pad08[4];
} Cnt;

typedef struct {
    /* 0x00 */ u32 head[2];
    /* 0x08 */ u32 tail[2];
} List;

typedef struct {
    /* 0x00 */ u8 pad00[0x18];
    /* 0x18 */ List list[3];
} Ctx;

/* callers read v0: no (callers 80175758/801757C4 ignore it; retail sets none); void. Cnt.base is a
 * guest address word. D_8011F7BC/BE are s16 as in the owner sources; retail reads them with lhu. */
extern Cnt D_800AF634[];
extern Ctx D_8011F7A8;
extern s16 D_8011F7BC;
extern s16 D_8011F7BE;
extern s32 func_80178004(u32 a, s16 x, s16 y);

void func_80176144(s16 i)
{
    u8 *p;
    u32 v;

    v = D_800AF634[i].base + D_800AF634[i].words * 4;
    p = (u8 *)&D_8011F7A8 + i * 4;
    *(u32 *)(p + 0x38) = v;
    /* one variable for the pointer and the call result keeps it in a0 */
    v = func_80178004(v, (u16)D_8011F7BC - 0x17, (u16)D_8011F7BE + 0x65);
    *(u32 *)(p + 0x40) = v - 0x14;
    v += 0x14;
    D_800AF634[i].words += (s32)(v - *(u32 *)(p + 0x38)) >> 2;
}
