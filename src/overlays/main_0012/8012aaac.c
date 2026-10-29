/* Owner re-derivation from retail assembly (clean input set). */
/* Applies the entry at obj->index of an object's frame sequence; negative entries are control codes (return/restart, return/stop, call, jump, reverse, skip) that act and recurse. */
#include "psx_types.h"

typedef struct {
    u32 value; /* +0: frame value, or the target sequence of a jump/call */
    s16 op;    /* +4: >= 0 hold time, < 0 control code */
    u8 pad6[2];
} Frame8012AAAC; /* 8 bytes */

typedef struct {
    u8 pad00[0x20];
    u32 target;    /* 0x20: guest pointer; the frame value goes to its +0x20 */
    u8 pad24[0x4E];
    u16 flags;     /* 0x72 */
    u8 pad74[0x1C];
    u32 frames;    /* 0x90: guest pointer to Frame8012AAAC[] */
    s32 index;     /* 0x94 */
    s16 delay;     /* 0x98 */
    s16 retIndex;  /* 0x9A: index to resume at after a call */
    u32 retFrames; /* 0x9C: guest pointer to the calling sequence, 0 if none */
} Obj8012AAAC;

void func_8012AAAC(Obj8012AAAC *obj)
{
    Frame8012AAAC *f;
    u32 saved;
    s32 next;

    obj->flags &= 0xBFFF;
    f = &((Frame8012AAAC *)obj->frames)[obj->index];
    if (f->op < 0) {
        switch (f->op) {
        case -2:
            if (obj->retFrames != 0) {
                obj->frames = obj->retFrames;
                obj->index = obj->retIndex;
                obj->retFrames = 0;
                obj->retIndex = 0;
                func_8012AAAC(obj);
            } else {
                obj->delay = 0;
            }
            break;
        case -1:
            if (obj->retFrames != 0) {
                obj->frames = obj->retFrames;
                obj->index = obj->retIndex;
                obj->retFrames = 0;
                obj->retIndex = 0;
                func_8012AAAC(obj);
            } else {
                obj->index = 0;
                func_8012AAAC(obj);
                obj->flags |= 0x4000;
            }
            break;
        case -4:
            obj->frames = f->value;
            obj->index = 0;
            obj->retFrames = 0;
            obj->retIndex = 0;
            obj->flags &= 0xF9FF;
            func_8012AAAC(obj);
            break;
        case -3:
            next = obj->index + 1;
            obj->index = next;
            if (obj->retFrames != 0) {
                func_8012AAAC(obj);
            } else {
                saved = obj->frames;
                obj->frames = f->value;
                obj->index = 0;
                obj->retFrames = 0;
                obj->retIndex = 0;
                obj->flags &= 0xF9FF;
                func_8012AAAC(obj);
                obj->retFrames = saved;
                obj->retIndex = next;
            }
            break;
        case -50:
            obj->index++;
            func_8012AAAC(obj);
            obj->flags |= 0x2000;
            break;
        case -5:
            obj->flags |= 0x400;
            obj->index--;
            func_8012AAAC(obj);
            break;
        }
    } else {
        *(u32 *)(obj->target + 0x20) = f->value;
        obj->delay = f->op;
    }
}
