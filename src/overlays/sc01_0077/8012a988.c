/* Owner re-derivation from retail assembly (clean input set). */
/* Per-object animation sequence tick: counts down the delay, steps/loops the frame index, then applies the frame. */
#include "psx_types.h"

typedef struct {
    u8 pad0[4];
    s16 hold; /* +4: -1 marks the end entry */
    u8 pad6[2];
} Frame8012A988; /* 8 bytes */

typedef struct {
    u16 active;       /* 0x00 */
    u8 pad02[0x70];
    u16 flags;        /* 0x72 */
    u8 pad74[0x1C];
    u32 frames;       /* 0x90: guest pointer to Frame8012A988[] */
    s32 index;        /* 0x94 */
    s16 delay;        /* 0x98 */
    u8 pad9A[0x27];
    u8 state;         /* 0xC1 */
} Obj8012A988;

extern s32 func_8012ACE0(Obj8012A988 *obj);
extern void func_8012AAAC(Obj8012A988 *obj);

void func_8012A988(Obj8012A988 *obj)
{
    s32 i;

    obj->flags &= 0x9FFF;
    if (obj->active == 0) {
        return;
    }
    if (obj->state == 0x11) {
        return;
    }
    if (obj->delay == 0) {
        return;
    }
    if (--obj->delay > 0) {
        return;
    }
    if (obj->frames == 0) {
        return;
    }

    if (obj->flags & 0x400) {
        if (--obj->index < 0) {
            obj->index = 1;
            obj->flags &= 0xFBFF;
        }
    } else if (obj->flags & 0x200) {
        if (--obj->index < 0) {
            i = func_8012ACE0(obj);
            if (((Frame8012A988 *)obj->frames)[i].hold != -1) {
                obj->index = 0;
                obj->delay = 0;
                return;
            }
            obj->index = i - 1;
        }
    } else {
        obj->index++;
    }
    func_8012AAAC(obj);
}
