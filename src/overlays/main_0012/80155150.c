/* Owner re-derivation from retail assembly (clean input set). */
/* Sets/clears the visibility bits of model parts 0x15/0x16/0x18/0x19 from the object's +0x44 state flags. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x24];
    u32 parts24;    /* 0x24: guest pointer to u32 part-flag table */
} Model80155150;

typedef struct {
    u8 pad00[0x20];
    u32 model20;    /* 0x20: guest pointer to Model80155150 */
    u8 pad24[0x20];
    u32 flags44;    /* 0x44 */
} Obj80155150;

extern void func_8001D130(u32 *parts, s32 idx);
extern void func_8001D150(u32 *parts, s32 idx);
extern s32 func_8014BEF8(Obj80155150 *obj);

#define PARTS(o) ((u32 *)((Model80155150 *)(o)->model20)->parts24)

void func_80155150(Obj80155150 *obj)
{
    u32 flags;

    func_8001D150(PARTS(obj), 0x19);
    func_8001D150(PARTS(obj), 0x18);
    func_8001D150(PARTS(obj), 0x16);
    func_8001D150(PARTS(obj), 0x15);

    flags = obj->flags44;
    if (flags & 0x400) {
        return;
    }
    if (!(flags & 0x20000)) {
        if (flags & 0x20000000) {
            func_8001D130(PARTS(obj), 0x19);
            func_8001D150(PARTS(obj), 0x18);
        } else if (func_8014BEF8(obj)) {
            func_8001D150(PARTS(obj), 0x19);
            func_8001D130(PARTS(obj), 0x18);
        } else {
            func_8001D150(PARTS(obj), 0x19);
            func_8001D150(PARTS(obj), 0x18);
        }
    }
    if (obj->flags44 & 0x10000000) {
        func_8001D130(PARTS(obj), 0x16);
        func_8001D150(PARTS(obj), 0x15);
    } else {
        func_8001D150(PARTS(obj), 0x16);
        func_8001D130(PARTS(obj), 0x15);
        if (obj->flags44 & 4) {
            func_8001D150(PARTS(obj), 0x15);
        }
    }
}
