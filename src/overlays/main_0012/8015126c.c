/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame action update for an actor: consumes the pending action word at 0x3C-0x42 and runs its handlers. */
#include "psx_types.h"

typedef struct {
    u16 type;
    u8 pad2[0x3C - 0x2];
    u16 act;
    u16 actId;
    s16 actArg;
    u16 actArg2;
    u32 f44;
    u8 pad48[0xA8 - 0x48];
    u8 actTimer;
    u8 padA9[0xDD - 0xA9];
    u8 lastAct;
    u8 padDE[0xF0 - 0xDE];
    u16 curId;
    s16 curArg;
    u16 curArg2;
    u8 padF6[0x1C8 - 0xF6];
    s16 f1C8;
    u8 pad1CA[0x1F8 - 0x1CA];
    u32 f1F8;
} Actor;

typedef struct {
    u8 pad0[8];
    u32 f8;
} Global800AE6A8;

typedef struct {
    u8 pad0[0x49];
    u8 mode;
} Global80078E78;

extern Global800AE6A8 D_800AE6A8;
extern Global80078E78 D_80078E78;
extern s32 D_801151FC;
extern s32 D_80126AE4;
extern s32 D_80127078;
extern s32 D_80127518;

extern s32 func_80029D3C(s32 arg);
extern void func_800CCF58(Actor *actor);
extern void func_800CCF68(Actor *actor);
extern void func_80146D80(Actor *actor);
extern void func_80147084(Actor *actor);
extern void func_801470A0(Actor *actor);
extern void func_80147324(s32 id);
extern void func_801477E8(Actor *actor, s32 arg);
extern s32 func_8014A454(Actor *actor);
extern void func_8014ACE8(Actor *actor, s32 a, s32 b);
extern void func_8014B310(Actor *actor);
extern void func_8014B4D4(Actor *actor);
extern void func_8014B944(Actor *actor, s32 a, s32 b);
extern void func_8014BD24(Actor *actor, s32 a);
extern s32 func_8014CB7C(void);
extern void func_8015173C(Actor *actor);
extern s32 func_80151880(Actor *actor);
extern s32 func_801518D8(Actor *actor);
extern void func_80153C9C(void);
extern void func_801599A4(Actor *actor);
extern void func_80160368(Actor *actor);
extern void func_801607B8(Actor *actor);
extern s32 func_801619D0(Actor *actor);
extern s32 func_80161A00(Actor *actor);
extern s32 func_80161A30(Actor *actor);
extern s32 func_80161A60(Actor *actor);
extern void func_801725CC(Actor *actor);

void func_8015126C(Actor *actor)
{
    Global800AE6A8 *g;
    Global80078E78 *m;
    u16 act;
    u16 id;
    u32 w;

    D_80126AE4 = D_80127078;
    D_80127078 = 0;
    D_801151FC = 0;
    m = &D_80078E78;
    g = &D_800AE6A8;
    if (actor->actTimer != 0) {
        u16 cur = actor->act;

        if (!(cur & 1) || !(actor->actId & 0x8000)) {
            actor->act = cur & 0xFFFE;
            actor->actId = 0;
            actor->actArg = 0;
            actor->actArg2 = 0;
        }
        actor->actTimer--;
        if (actor->actTimer & 1) {
            func_801470A0(actor);
        }
    }
    act = actor->act;
    if (act & 1) {
        if (!(*(u32 *)&actor->act & 0x80008000) || (g->f8 & 0x80FFFFFF)) {
            actor->act = act & 0xFFFE;
            actor->actId = 0;
            actor->actArg = 0;
            actor->actArg2 = 0;
        } else {
            actor->act = act & 0xFFFE;
            actor->actId &= 0x7FFF;
            if (!(actor->actId & 0x4000) && (actor->f44 & 1) && func_8014A454(actor)) {
                func_80160368(actor);
                actor->actId = 0;
                actor->actArg = 0;
                actor->actArg2 = 0;
                actor->act &= 0xFFFE;
                return;
            }
            if (func_801619D0(actor) && func_80151880(actor)) {
                actor->actArg = 0;
                actor->curArg = 0;
                func_800CCF68(actor);
            }
            if (func_80161A00(actor) && func_801518D8(actor)) {
                func_800CCF58(actor);
            }
            if (func_80161A30(actor)) {
                actor->f1C8 = 0;
            }
            if (func_80161A60(actor)) {
                actor->f1C8 = 0;
            }
            if (actor->type != 0x19 && actor->type != 0x1A
                && ((w = actor->f1F8) == 0 || (w & 0x2000000))) {
                actor->curId = actor->actId;
                actor->curArg = actor->actArg;
                actor->curArg2 = actor->actArg2;
                func_801599A4(actor);
                func_801607B8(actor);
                func_80147084(actor);
                id = actor->actId;
                actor->actTimer = 0;
                actor->lastAct = id;
                if ((u8)id != 0x11 && actor->curArg != 1000) {
                    actor->curArg = func_80029D3C(actor->curArg);
                    if (m->mode == 5) {
                        actor->curArg = actor->curArg / 2;
                    } else if (func_8014CB7C()) {
                        actor->curArg = actor->curArg / 2;
                    }
                }
                func_80153C9C();
                func_8015173C(actor);
                func_801725CC(actor);
                if (actor->actId & 0x2000) {
                    func_8014B310(actor);
                }
                if (actor->actId & 0x1000) {
                    func_8014B4D4(actor);
                }
                if (actor->actId & 0x800) {
                    func_8014B944(actor, 0x50000, 1);
                }
                if (actor->actId & 0x400) {
                    func_80146D80(actor);
                    func_801477E8(actor, 0);
                }
                if (func_80151880(actor) == 2) {
                    func_80147324(0x676);
                }
                if (m->mode == 0x14) {
                    func_8014BD24(actor, 10);
                    func_8014ACE8(actor, 4, 10);
                }
                D_80127518 = D_80126AE4;
                D_801151FC = D_80127518;
                return;
            } else {
                actor->actId = 0;
                actor->actArg = 0;
                actor->actArg2 = 0;
                actor->act &= 0xFFFE;
            }
        }
    }
    D_801151FC = D_80127518;
}
