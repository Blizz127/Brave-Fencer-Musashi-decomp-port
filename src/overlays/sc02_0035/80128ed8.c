/* Owner re-derivation from retail assembly (clean input set). */
/* Steps a frame-table animator: counts down the hold timer, then advances to the next table entry (stop/loop flags in bits 6-7) and loads its hold time and two 16-bit outputs into the object. Returns 1 when finished/idle. */
#include "psx_types.h"
struct AnimEnt {
    u16 x;
    u16 y;
    u32 flags;
};
struct AnimSt {
    struct AnimEnt *tbl;
    short idx;
    short hold;
};
int func_80128ED8(char *o, struct AnimSt *st) {
    int pad[1];
    short c = st->hold;
    struct AnimEnt *t = st->tbl;
    u32 fl;
    if (c == 0) {
        return 1;
    }
    c = c - 1;
    st->hold = c;
    if (c <= 0) {
        fl = t[st->idx].flags;
        if ((fl & 0xC0) == 0xC0) {
            st->hold = 0;
            return 1;
        }
        if (fl & 0x80) {
            st->idx = 0;
        } else {
            st->idx = st->idx + 1;
        }
        st->hold = t[st->idx].flags & 0x3F;
        *(u16 *)(o + 0x28) = t[st->idx].x;
        *(u16 *)(o + 0x2A) = t[st->idx].y;
    }
    return 0;
}
