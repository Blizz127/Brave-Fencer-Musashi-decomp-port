/* Owner re-derivation from retail assembly (clean input set). */
/* Draws the window list for the mode D_8011512C (0..12): the mode selects the window count n (1 or 5) and whether one extra record at offset 0xA0 is drawn first (modes 3, 4, 6, 7, 10, 11, 12); modes 1 and 2 first run func_8017DD28 on the cursor. Then appends a draw-mode packet (func_8005A600) and links it into the current ordering table, advancing the cursor D_801151D0 by 0x28. For a mode above 12 the count n is never set (retail reads a stale register there). */
#include "psx_types.h"

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

typedef struct {
    u32 ot;
    u8 pad04[0x10];
} Buf;

typedef struct {
    u32 w0;
    u32 w1;
    PrimTag tag;
} OtHead;

typedef struct {
    u16 idx;
} Cur;

extern Cur D_800B9A02;
extern Buf D_800AE7BC[];
extern u32 D_801151D0;
extern u16 D_8011512C;
extern u32 D_80115134;

extern u32 func_8017DD28(u32 p);
extern u32 func_8017CBC8(u32 p, u32 w, s16 sel);
extern void func_8005A600(u32 p, s32 b, s32 c, u16 d, s32 e);

void func_8017CA18(void)
{
    u32 p;
    u32 w;
    s16 n;
    s16 i;

    p = D_801151D0;
    w = D_80115134;
    if ((u32)D_8011512C - 1 < 2) {
        p = func_8017DD28(p);
    }
    switch ((s16)D_8011512C) {
    case 0:
    case 5:
    case 8:
    case 9:
        n = 1;
        break;
    case 1:
    case 2:
        n = 5;
        break;
    case 3:
    case 4:
    case 10:
    case 11:
        n = 5;
        goto extra;
    case 6:
    case 7:
    case 12:
        n = 1;
    extra:
        p = func_8017CBC8(p, w + 0xA0, 0);
        break;
    }
    for (i = 0; i < n; i++, w += 0x20) {
        p = func_8017CBC8(p, w, i);
    }
    func_8005A600(p, 0, 0, 0x15, 0);
    *(u32 *)p = 0x2000000;
    ((PrimTag *)p)->addr = ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr;
    *(u32 *)(D_800AE7BC[D_800B9A02.idx].ot + 8) = (*(u32 *)(D_800AE7BC[D_800B9A02.idx].ot + 8) & 0xFF000000) | ((u32)p & 0xFFFFFF);
    p += 0x28;
    D_801151D0 = p;
}
