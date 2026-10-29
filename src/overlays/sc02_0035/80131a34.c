/* Owner re-derivation from retail assembly (clean input set). */
/* Looks key k up in o's +0xBC table; if found, sets or clears bit 4 of +0xC4 * (set for keys 0xB, 8, 0x20) and hands the value to func_8012A828. */
#include "psx_types.h"

extern s32 func_80131CF4(void *p, s32 key);
extern s32 func_8012A828(u8 *o, u32 a);

/* Callers read v0. func_80131CF4 takes (table, key): k stays in a1 for it. */
s32 func_80131A34(u8 *o, s32 k) {
    s32 r;
    r = func_80131CF4(*(void **)(o + 0xBC), k);
    if (r != 0) {
        if (k == 0xB || k == 8 || k == 0x20) {
            *(u32 *)(o + 0xC4) = *(u32 *)(o + 0xC4) | 4;
        } else {
            *(u32 *)(o + 0xC4) = *(u32 *)(o + 0xC4) & ~4;
        }
        func_8012A828(o, r);
        return 1;
    }
    return 0;
}
