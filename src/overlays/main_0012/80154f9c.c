/* Owner re-derivation from retail assembly (clean input set). */
/* Toggles bit 24 of the object's +0xC8 flags from global input state and posts the matching request via func_801550FC. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x44];
    u32 flags44;    /* 0x44 */
    u8 pad48[0x80];
    u32 flagsC8;    /* 0xC8 */
} Obj80154F9C;

typedef struct {
    u8 pad00[0x1E];
    s16 h1E;        /* 0x1E */
    u8 pad20[0x6];
    s16 h26;        /* 0x26 */
    u8 pad28[0x6];
    s16 h2E;        /* 0x2E */
} Glob80154F9C;

extern Glob80154F9C D_80078E78;
extern u8 D_800D8D10[];
extern s32 func_8016F1AC(void);
extern void func_801550FC(u32 req);

void func_80154F9C(Obj80154F9C *obj)
{
    Glob80154F9C *g = &D_80078E78;

    if (obj->flagsC8 & 0x01000000) {
        if ((g->h1E & 0x8000) || g->h26 < 0x10 || g->h2E < 0x10) {
            if (func_8016F1AC() == 0) {
                return;
            }
        }
        func_801550FC(obj->flagsC8 &= 0xC0FFFFFF);
    } else if ((g->h1E & 0x8000) && func_8016F1AC() == 0) {
        obj->flagsC8 |= 0x01000000;
        func_801550FC((u32)D_800D8D10);
    } else if ((g->h26 < 0x10 || g->h2E < 0x10) && !(obj->flags44 & 0xC0)
               && func_8016F1AC() == 0) {
        obj->flagsC8 |= 0x01000000;
        func_801550FC(0x40000002);
    }
}
