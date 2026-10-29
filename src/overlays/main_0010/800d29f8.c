/* Owner re-derivation from retail assembly (clean input set). */
/* Builds a draw-mode packet, up to n gouraud quads from 16-byte records (two colour words, then x, y, w, h halfwords; records whose index is not in the selected pair or is odd below 8 are skipped), and a closing draw-mode packet, linking each into the current ordering table. Returns the next free primitive. */
#include "psx_types.h"

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

typedef struct {
    PrimTag tag;
    u32 code;
} DrMode;

typedef struct {
    u32 tag;
    u32 c0;
    s16 x0, y0;
    u32 c1;
    s16 x1, y1;
    u32 c2;
    s16 x2, y2;
    u32 c3;
    s16 x3, y3;
} PolyG4;

typedef struct {
    u32 ot; /* guest pointer to the table's head words */
    u8 pad04[0x10];
} Buf80175820; /* 0x14 */

typedef struct {
    u32 w0;
    u32 w1;
    PrimTag tag;
} OtHead;

typedef struct {
    u16 idx;
} Cur80175820;

extern Cur80175820 D_800B9A02;
extern Buf80175820 D_800AE7BC[];

DrMode *func_800D29F8(s16 sel, PolyG4 *p, u8 *src, s16 n, u16 xoff)
{
    s16 i;
    s32 dx;
    s32 k;
    s32 x;
    u16 y;
    s32 w;
    s32 h;
    u32 c;
    u32 c2;

    dx = xoff;
    i = 0;
    ((DrMode *)p)->tag.len = 1;
    ((DrMode *)p)->code = 0xE1000000;
    ((PrimTag *)p)->addr = ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr;
    ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr = (u32)p;
    p = (PolyG4 *)((u8 *)p + 8);
    if (n > 0) {
        do {
            if ((i >> 1) == sel || i >= 8) {
                k = 0;
            } else if (i & 1) {
                k = 2;
            } else {
                src += 0x10;
                goto next;
            }
            p->tag = 0x08000000;
            c = ((k | 0x38) << 24) | *(u32 *)src;
            src += 4;
            p->c0 = c;
            p->c1 = c + 0xFFD80000;
            c2 = *(u32 *)src;
            src += 4;
            p->c3 = c2;
            p->c2 = c2 + 0x280000;
            x = *(u16 *)src;
            x += dx;
            src += 2;
            p->x0 = p->x2 = x;
            y = *(u16 *)src;
            src += 2;
            p->y1 = y;
            p->y0 = y;
            w = *(u16 *)src;
            src += 2;
            h = *(u16 *)src;
            src += 2;
            w = w + x;
            y = y + h;
            p->x3 = w;
            p->x1 = w;
            p->y3 = y;
            p->y2 = y;
            ((PrimTag *)p)->addr = ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr;
            ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr = (u32)p;
            p++;
next:
            i++;
        } while (i < n);
    }
    ((DrMode *)p)->tag.len = 1;
    ((DrMode *)p)->code = 0xE1000200;
    ((PrimTag *)p)->addr = ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr;
    ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr = (u32)p;
    return (DrMode *)p + 1;
}
