/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame update of the 0x60-entry object table: dispatches each live object's handler and syncs its child's position. */
#include "psx_types.h"

typedef union {
    s32 w;
    struct {
        u16 lo;
        u16 hi;
    } h;
} Fx8017849C;

typedef struct {
    Fx8017849C x;
    Fx8017849C y;
    Fx8017849C z;
} Vec8017849C; /* 0x0C */

typedef struct {
    /* 0x00 */ u8 pad00[0x8];
    /* 0x08 */ s16 x;
    /* 0x0A */ s16 y;
    /* 0x0C */ s16 z;
} Child8017849C;

typedef struct {
    /* 0x000 */ u16 type;
    /* 0x002 */ u8 pad002[0x2];
    /* 0x004 */ Vec8017849C pos;
    /* 0x010 */ u8 pad010[0x10];
    /* 0x020 */ u32 child;
    /* 0x024 */ u8 pad024[0x14];
    /* 0x038 */ Vec8017849C prevPos;
    /* 0x044 */ u8 pad044[0xC];
    /* 0x050 */ u16 offX;
    /* 0x052 */ u16 offY;
    /* 0x054 */ u16 offZ;
    /* 0x056 */ u8 pad056[0x1C];
    /* 0x072 */ u16 flags;
    /* 0x074 */ u8 pad074[0x4C];
    /* 0x0C0 */ u8 altA;
    /* 0x0C1 */ u8 altB;
    /* 0x0C2 */ u8 pad0C2[0x4A];
} Obj8017849C; /* 0x10C */

extern Obj8017849C D_801202A0[];
extern s16 D_801270C0;
extern u32 D_8011DB08;

extern void func_801787D4(void);
extern void func_8012A908(void);
extern void func_8017869C(Obj8017849C *obj);
extern void func_80130D0C(Obj8017849C *obj);
extern void func_8013E370(void);

void func_8017849C(void)
{
    Obj8017849C *obj;
    u32 *tbl;
    s32 i;

    obj = D_801202A0;
    func_801787D4();
    func_8012A908();
    for (i = 0; i < 0x60; i++, obj++) {
        if (obj->type == 0) {
            continue;
        }
        if (D_801270C0 != 1 && !(obj->flags & 0x1000)) {
            continue;
        }
        func_8017869C(obj);
        obj->prevPos.x.w = obj->pos.x.w;
        obj->prevPos.y.w = obj->pos.y.w;
        obj->prevPos.z.w = obj->pos.z.w;
        tbl = (u32 *)D_8011DB08; /* guest handler table: u32 elements, cast at use */
        if (obj->altA != 0 && obj->altB != 0) {
            func_80130D0C(obj);
        } else {
            ((void (*)(Obj8017849C *))tbl[obj->type])(obj);
        }
        if (obj->child != 0) {
            ((Child8017849C *)obj->child)->x = obj->pos.x.h.hi + obj->offX;
            ((Child8017849C *)obj->child)->y = obj->pos.y.h.hi + obj->offY;
            ((Child8017849C *)obj->child)->z = obj->pos.z.h.hi + obj->offZ;
        }
    }
    if (D_801270C0 == 1) {
        func_8013E370();
    }
}
