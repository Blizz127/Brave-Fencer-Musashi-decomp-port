/* Owner re-derivation from retail assembly (clean input set). */
/* Spins the sub-object's yaw (+0x16 per tick); when func_801399F0 reports an event on handle +0x198,
 * releases it, picks the model for result 1/2 and re-creates the handle, then calls func_80146CA0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x12];
    u16 yaw; /* 0x12 */
} Sub80159070;

typedef struct {
    u8 pad0[0x20];
    u32 sub;     /* 0x20: guest address of Sub80159070 */
    u8 pad24[0x174];
    u32 handle;  /* 0x198 */
    u8 pad19C[0xA8];
    s16 result;  /* 0x244 */
} Obj80159070;

extern u8 D_80110C94[];
extern u8 D_80110CD4[];
extern s16 func_801399F0(u32 h);
extern void func_80139914(u32 h);
extern s32 func_8013767C(u8 *p);
extern void func_80146CA0(Obj80159070 *o);

void func_80159070(Obj80159070 *o)
{
    Sub80159070 *sub;
    u8 *model;

    sub = (Sub80159070 *)o->sub;
    sub->yaw = (sub->yaw + 0x16) & 0xFFF;
    o->result = func_801399F0(o->handle);
    if (o->result != 0) {
        func_80139914(o->handle);
        o->handle = 0;
        switch (o->result) {
        case 1:
        default:
            model = D_80110C94;
            break;
        case 2:
            model = D_80110CD4;
            break;
        }
        o->handle = func_8013767C(model);
        func_80146CA0(o);
    }
}
