/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame player update: runs the sub-steps, syncs the model position, latches previous state. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ s16 x;
    /* 0x02 */ s16 y;
    /* 0x04 */ s16 z;
    /* 0x06 */ s16 pad;
} Vec801463A0; /* size 0x8 */

typedef struct {
    /* 0x00 */ u8 pad00[6];
    /* 0x06 */ s16 x;
    /* 0x08 */ u8 pad08[2];
    /* 0x0A */ s16 y;
    /* 0x0C */ u8 pad0C[2];
    /* 0x0E */ s16 z;
    /* 0x10 */ u8 pad10[0x10];
    /* 0x20 */ u32 model; /* guest pointer */
    /* 0x24 */ u8 pad24[0x20];
    /* 0x44 */ u32 state;
    /* 0x48 */ u32 prevState;
    /* 0x4C */ u8 pad4C[0x3C];
    /* 0x88 */ Vec801463A0 pos;
    /* 0x90 */ Vec801463A0 prevPos;
    /* 0x98 */ u8 pad98[0xA0];
    /* 0x138 */ s16 offX;
    /* 0x13A */ s16 offY;
    /* 0x13C */ s16 offZ;
    /* 0x13E */ u8 pad13E[8];
    /* 0x146 */ s16 f146;
} Player801463A0;

typedef struct {
    /* 0x00 */ u8 pad00[8];
    /* 0x08 */ s16 x;
    /* 0x0A */ s16 y;
    /* 0x0C */ s16 z;
    /* 0x0E */ u8 pad0E[0x1E];
    /* 0x2C */ u16 flags;
    /* 0x2E */ u8 pad2E[0x1A];
    /* 0x48 */ s32 wx;
    /* 0x4C */ s32 wy;
    /* 0x50 */ s32 wz;
} Model801463A0;

extern Player801463A0 D_80126B58;

extern void func_8014C6F4(Player801463A0 *p);
extern void func_80155150(Player801463A0 *p);
extern void func_801470C0(Player801463A0 *p);
extern void func_80147478(Player801463A0 *p);
extern void func_80147118(Player801463A0 *p);
extern void func_8014BDE8(Player801463A0 *p);
extern s32 func_801508F8(Player801463A0 *p);
extern void func_8014B5B0(Player801463A0 *p);
extern void func_80161D88(Player801463A0 *p);

void func_801463A0(void)
{
    Player801463A0 *p;
    Model801463A0 *model;

    p = &D_80126B58;
    model = (Model801463A0 *)p->model;
    func_8014C6F4(p);
    func_80155150(p);
    func_801470C0(p);
    func_80147478(p);
    func_80147118(p);
    if (model != 0) {
        model->x = p->x + p->offX;
        model->wx = model->x;
        model->y = p->y + p->offY;
        model->wy = model->y;
        model->z = p->z + p->offZ;
        model->wz = model->z;
        model->flags |= 0x11;
    }
    func_8014BDE8(p);
    p->prevPos = p->pos;
    p->pos.x = p->x;
    p->pos.y = p->y;
    p->pos.z = p->z;
    p->prevState = p->state;
    p->state &= 0x3FFFFFFF;
    func_801508F8(p);
    p->f146 = 0;
    func_8014B5B0(p);
    func_80161D88(p);
}
