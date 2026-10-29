/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame player update: setup, mode/flag select, runs the update chain unless blocked; returns 1 if it ran. */
#include "psx_types.h"

typedef struct {
    /* 0x000 */ u8 pad000[4];
    /* 0x004 */ s32 pos[3];
    /* 0x010 */ u8 pad010[0x34];
    /* 0x044 */ u32 flags;
    /* 0x048 */ u8 pad048[0x50];
    /* 0x098 */ s16 spos[3];
    /* 0x09E */ u8 pad09E[0x112];
    /* 0x1B0 */ s32 timer;
} Obj80146128;

typedef struct {
    /* 0x00 */ u8 pad00[2];
    /* 0x02 */ s16 mode;
} Sub80146128;

typedef struct {
    /* 0x00 */ u8 b[8];
} Pair80146128;

extern Obj80146128 D_80126B58;
extern u32 D_80126B78; /* guest pointer: Sub80146128 */
extern u32 D_80126B9C;
extern Pair80146128 D_80126BE0;
extern Pair80146128 D_801150F0;
extern s32 D_8011F730;
extern s16 D_801152B8;
extern s16 D_8012693A;
extern u8 D_80078EC1;
extern s32 D_80078EC8;

extern void func_80146FC4(Obj80146128 *obj);
extern void func_80150A70(Obj80146128 *obj);
extern void func_80147098(Obj80146128 *obj);
extern void func_8014A638(Obj80146128 *obj);
extern s32 func_80155458(Obj80146128 *obj);
extern u8 func_80029104(void);
extern void func_8014ADE0(Obj80146128 *obj);
extern void func_8014B350(Obj80146128 *obj);
extern void func_8014B7A4(Obj80146128 *obj);
extern void func_80161D58(Obj80146128 *obj);
extern void func_80029344(void);
extern void func_80161A90(Obj80146128 *obj);
extern void func_8014B504(Obj80146128 *obj);
extern void func_80149BEC(Obj80146128 *obj);
extern void func_8014B5D0(Obj80146128 *obj);
extern void func_8014C99C(Obj80146128 *obj);
extern void func_8014B190(Obj80146128 *obj);
extern void func_80148648(Obj80146128 *obj, u8 port);
extern void func_80149228(Obj80146128 *obj);
extern s32 func_8014A59C(Obj80146128 *obj);
extern void func_8016F14C(Obj80146128 *obj);
extern void func_80154418(Obj80146128 *obj);
extern void func_80154BE4(Obj80146128 *obj);
extern void func_80165694(Obj80146128 *obj);
extern void func_801654A8(Obj80146128 *obj);
extern void func_8014A680(Obj80146128 *obj);
extern void func_8014A6A8(Obj80146128 *obj);
extern void func_8014A71C(Obj80146128 *obj);
extern void func_80172588(Obj80146128 *obj);
extern void func_801473DC(Obj80146128 *obj);
extern u16 func_80015978(s32 *src, s16 *dst);

s32 func_80146128(void)
{
    Obj80146128 *obj;
    Sub80146128 *sub;

    obj = &D_80126B58;
    sub = (Sub80146128 *)D_80126B78;
    func_80146FC4(obj);
    D_801150F0 = D_80126BE0;
    func_80150A70(obj);
    func_80147098(obj);
    func_8014A638(obj);
    D_8011F730 = 0;
    D_801152B8 = 0;
    D_8012693A = 0;
    if (D_80078EC1 == 0x10 && D_80078EC8 != 0) {
        *(s16 *)((u8 *)sub + 2) = 6; /* same field as sub->mode */
        D_80126B9C |= 0x800;
    } else {
        sub->mode = 1;
        obj->flags &= ~0x800;
    }
    if (obj->timer != 0) {
        obj->timer--;
    }
    if (func_80155458(obj) == 0 && !(obj->flags & 0x4000000)) {
        if (func_80029104() != 0) {
            func_8014ADE0(obj);
            func_8014B350(obj);
            func_8014B7A4(obj);
            func_80161D58(obj);
        }
        func_80029344();
        func_80161A90(obj);
        func_8014B504(obj);
        func_80149BEC(obj);
        func_8014B5D0(obj);
        func_8014C99C(obj);
        func_8014B190(obj);
        func_80148648(obj, 0);
        func_80149228(obj);
        func_8014A59C(obj);
        func_8016F14C(obj);
        func_80154418(obj);
        func_80154BE4(obj);
        func_80165694(obj);
        func_801654A8(obj);
        func_8014A680(obj);
        func_8014A6A8(obj);
        func_8014A71C(obj);
        func_80172588(obj);
        func_801473DC(obj);
        func_80015978(obj->pos, obj->spos);
        return 1;
    }
    func_80015978(obj->pos, obj->spos);
    return 0;
}
