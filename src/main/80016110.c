/* func_80016110: queue a full-screen black semi-transparent POLY_F4 (-160..160, -120..120) into the current
 * ordering table (D_800A6518[D_800B9A02].ot), then call func_80016638 for the current buffer.
 * Idioms (gcc 2.7.2 sched1): every field store is a non-struct (cast) store, so none of them may move below the
 * D_800B9A02 load; the tag link reads the prim word into a variable that is set three times (not a "birthing"
 * insn), so sched1 issues it right after the index load and defers the 0xFF000000 constant, which gives the
 * retail a0/a1 allocation. The OT links are plain u32 read-modify-writes (a bitfield link lets CSE keep the
 * index instead of reloading it three times). */
#include "psx_types.h"

typedef struct {
    u32 addr : 24;
    u32 len : 8;
} PrimTag;

typedef struct {
    PrimTag tag;
    u8 r, g, b, code;
    s16 x0, y0;
    s16 x1, y1;
    s16 x2, y2;
    s16 x3, y3;
} PolyF4;

typedef struct {
    u32 unused;
    u32 ot; /* guest PrimTag * */
    u8 pad[0x14 - 8];
} OtBuffer;

extern void *func_80010A08(s32 size);
extern void func_80016638(OtBuffer *ot, s32 idx, s32 mode);
extern OtBuffer D_800A6518[];
extern u16 D_800B9A02;

/* byte / halfword store at offset o of the prim, as non-struct memory */
#define PB(o) (*(u8 *)((u8 *)p + (o)))
#define PH(o) (*(s16 *)((u8 *)p + (o)))

void func_80016110(void) {
    PolyF4 *p;
    u32 tag;

    p = func_80010A08(0x18);
    PB(3) = 5; /* len */
    PB(7) = 0x2A; /* code */
    PH(0x10) = -160; /* x2 */
    PH(0x08) = -160; /* x0 */
    PH(0x14) = 160; /* x3 */
    PH(0x0C) = 160; /* x1 */
    PH(0x0E) = -120; /* y1 */
    PH(0x0A) = -120; /* y0 */
    PB(6) = 0; /* b */
    PB(5) = 0; /* g */
    PB(4) = 0; /* r */
    PH(0x16) = 120; /* y3 */
    PH(0x12) = 120; /* y2 */
    tag = *(u32 *)p;
    tag &= 0xFF000000;
    tag |= *(u32 *)D_800A6518[D_800B9A02].ot & 0xFFFFFF;
    *(u32 *)p = tag;
    *(u32 *)D_800A6518[D_800B9A02].ot = (*(u32 *)D_800A6518[D_800B9A02].ot & 0xFF000000) | ((u32)p & 0xFFFFFF);
    func_80016638(&D_800A6518[D_800B9A02], 0, 0);
}
