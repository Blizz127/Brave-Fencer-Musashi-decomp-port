/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the sc01_0005 twin at this address (W18). */
/* Expands a 12-row, 8-pixel-wide 1bpp glyph (12 source bytes starting at the row's cell) into 4bpp
 * pixels of a given color, merges it into the 6-pixel cell staging area picked by the cursor, then
 * uploads the staged cell through func_800599B8. Returns 1 when the cursor query fails, else 0.
 * The accumulator is intentionally not initialised: the first nibbles shifted into it are
 * discarded by the 16-bit truncation before the first store, so its entry value never reaches memory. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x23];
    u8 color;   /* 0x23: low nibble is the pixel color */
    u8 pad24[0x14];
    u16 base;   /* 0x38 */
} Obj;

/* Rectangle handed to the video upload routine. */
typedef struct {
    u16 x;
    u16 y;
    s16 w;
    s16 h;
} Rect;

/* Staging counter followed by the staging pixels. */
typedef struct {
    s32 n;
    u8 data[1];
} Stage;

extern u8 D_80127C0C[];
extern Stage D_80127548;

extern s32 func_8013914C(Obj *o, Rect *r);
extern void func_800599B8(Rect *r, u8 *data);

#undef PACK
#define PACK(lo, hi)                                  \
    for (j = (lo); j < (hi); j++) {                   \
        t = (u16)acc << 4;                            \
        if (!((b >> j) & 1)) {                        \
            acc = t;                                  \
        } else {                                      \
            acc = t | cm;                              \
        }                                             \
    }

s32 func_80138ED0(Obj *o, u16 row, u8 *src)
{
    Rect r;
    s32 acc;
    u8 tmp;
    u16 c;
    u8 b;
    s32 j;
    s32 t;
    u16 idx;
    u16 *d;
    u16 *dc;
    u16 *d0;
    u8 *s;
    u8 *st;
    s32 i;
    s32 cm;

    tmp = o->color & 0xF;
    c = tmp;
    if (func_8013914C(o, &r) != 0) {
        return 1;
    }
    idx = r.x - o->base;
    s = src + row * 12;
    d0 = (u16 *)D_80127C0C + idx;
    if ((u16)(idx % 3) != 0) {
        i = 0;
        cm = c;
        d = d0;
        for (; i < 12; i++) {
            b = *s;
            PACK(6, 8)
            d[0] |= acc << 8;
            d++;
            b = *s;
            PACK(2, 6)
            d[0] = acc;
            d = (u16 *)((u8 *)d + 0x6A);
            s++;
        }
    } else {
        i = 0;
        cm = c;
        d = d0;
        for (; i < 12; i++) {
            b = *s;
            PACK(4, 8)
            d[0] = acc;
            d++;
            b = *s;
            PACK(2, 4)
            d[0] = acc & 0xFF;
            d = (u16 *)((u8 *)d + 0x6A);
            s++;
        }
    }
    st = D_80127548.data + (0x24 - D_80127548.n) * 0x30;
    dc = (u16 *)st;
    {
        u16 *p;
        i = 0;
        p = d0;
    again:
        dc[0] = p[0];
        p++;
        dc++;
        dc[0] = p[0];
        p = (u16 *)((u8 *)p + 0x6A);
        dc++;
        i++;
        if (i < 12) goto again;
    }
    r.w = 2;
    r.h = 12;
    func_800599B8(&r, st);
    return 0;
}
