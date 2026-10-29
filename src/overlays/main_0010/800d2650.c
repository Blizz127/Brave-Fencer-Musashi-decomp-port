/* Owner re-derivation from retail assembly (clean input set). */
/* Text run builder: walks (u, v) byte pairs while the first byte is below 0x80 and emits one 8x8 textured sprite
 * (0x14-byte primitive: tag, colour word, x, y, u, v, clut, w, h) per glyph, linking each into the current ordering
 * table. The pairs 0x3870, 0x3871 and 0x3872 are blanks that only advance x by 7, 4 and 2; the pairs 0x1850 and 0x1858
 * draw a mark at (x + 4, y - 7) without advancing; every other pair draws at (x, y) and advances x by 7.
 * Returns the next free primitive. */
#include "psx_types.h"

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

typedef struct {
    u32 tag;
    u32 rgbc;
    s16 x, y;
    u8 u, v;
    u16 clut;
    u16 w, h;
} Sprt;

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

Sprt *func_800D2650(Sprt *p, u8 *text, s16 x, s16 y, s16 base, u32 color)
{
    while (*text < 0x80) {
        switch (*(s16 *)text) {
        case 0x1850:
        case 0x1858:
            p->x = x + 4;
            p->y = y - 7;
            break;
        case 0x3870:
            x += 7;
            text += 2;
            continue;
        case 0x3871:
            x += 4;
            text += 2;
            continue;
        case 0x3872:
            x += 2;
            text += 2;
            continue;
        default:
            p->x = x;
            p->y = y;
            x += 7;
            break;
        }
        p->tag = 0x04000000;
        p->rgbc = color | 0x64000000;
        p->u = *text++;
        p->v = *text++;
        p->clut = ((base + 0x100) << 6) | 0x16;
        p->w = 8;
        p->h = 8;
        ((PrimTag *)p)->addr = ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr;
        ((OtHead *)D_800AE7BC[D_800B9A02.idx].ot)->tag.addr = (u32)p;
        p++;
    }
    return p;
}
