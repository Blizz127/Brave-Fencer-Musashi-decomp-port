/* Step the player model sequence, preserving command effects and recursive flag accumulation. */
#include "mine_model_sequence.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define POSE_ENTRY(word) ((word) + 0x10)
#define SCRIPT(actor) ((MineSequenceWord *)(actor)->script)
#else
/* Numeric original tokens wrap at 32 bits. Relocated native pose pointers
 * keep their full width when advancing past the 16-byte frame header. */
static MineSequenceWord mine_model_pose(MineSequenceWord word)
{
#if UINTPTR_MAX > UINT32_MAX
    if (word <= UINT32_MAX) return (u32)((u32)word + 16u);
#endif
    return word + 16u;
}
#define POSE_ENTRY(word) mine_model_pose(word)
#define SCRIPT(actor) ((const MineSequenceWord *)(actor)->script)
#endif

void func_8015444C(MineSequenceActor *o, s32 *cnt, u32 *fl, s32 *skip)
{
    MineSequenceModel *h = (MineSequenceModel *)o->model;
    u8 b;
    MineSequenceWord cmd;
    MineSequenceWord nxt;
    u32 npc;

    if (h == 0) {
        return;
    }
    b = o->timer;
    if (b == 0) {
        return;
    }
    if (*cnt == 0) {
        o->timer = b;
        *cnt = *cnt + 1;
    }
    o->timer--;
    if ((u8)o->timer != 0) {
        goto done;
    }
    cmd = SCRIPT(o)[o->pc];
    switch (cmd) {
    case 0:
        o->timer = 0;
        *fl |= 0x8000;
        o->saved = 0;
        o->pc = 0;
        o->depth = 0;
        break;
    case 1:
        o->timer = 1;
        *fl |= 0x8000;
        o->pc = 0;
        o->depth = 0;
        o->dc = 0;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 2:
        o->timer = 0;
        o->pc = o->pc + 1;
        *fl |= 0x8000;
        break;
    case 3:
        nxt = SCRIPT(o)[o->pc + 1];
        o->timer = 1;
        o->pc = o->pc + 2;
        o->saved = nxt;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 4:
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 5:
        nxt = SCRIPT(o)[o->pc + 1];
        npc = SCRIPT(o)[o->pc + 2];
        o->timer = 1;
        o->dc = 0;
        o->pc = npc;
        o->script = (MineSequenceScriptRef)nxt;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 6:
        npc = SCRIPT(o)[o->pc + 1];
        o->timer = 1;
        o->pc = npc;
        o->dc = func_801549F8(o);
        func_8015444C(o, cnt, fl, skip);
        break;
    case 7:
        o->timer = 1;
        *fl |= 0x4000;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 8:
        o->timer = 1;
        *fl |= 0x2000;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 9:
        o->timer = 1;
        *fl |= 0x1000;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 10:
        o->timer = 1;
        *fl |= 0x800;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 11:
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 12:
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 13:
        o->timer = 1;
        o->pc = o->pc + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 14:
        if (*skip == 0) {
            func_80154B4C(o, SCRIPT(o)[o->pc + 1]);
        }
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 15:
        if (*skip == 0) {
            func_80154A74(o, SCRIPT(o)[o->pc + 1]);
        }
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 16:
        if (*skip == 0) {
            func_801553C0(o);
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 17:
        if (*skip == 0) {
            func_80155440(o);
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 18:
        if (*skip == 0) {
            func_80155394(o);
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 19:
        if (*skip == 0) {
            func_801553A8(o);
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 20:
        if (*skip == 0) {
            o->b1A8 = o->b1A8 ^ 1;
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 21:
        if (*skip == 0) {
            func_80059A1C(SCRIPT(o)[o->pc + 1], D_80126738);
        }
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 22:
        if (*skip == 0) {
            func_80059A1C(SCRIPT(o)[o->pc + 1], D_80126838);
        }
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 23:
        if (*skip == 0) {
            h->pose = (MineSequenceWord)D_80126748;
        }
        o->timer = o->saved;
        o->pc++;
        o->depth++;
        o->dc++;
        break;
    case 24:
        if (*skip == 0) {
            h->pose = (MineSequenceWord)D_80126848;
        }
        o->timer = o->saved;
        o->pc++;
        o->depth++;
        o->dc++;
        break;
    case 25:
        if (*skip == 0) {
            o->flags &= 0xFFFDFFFF;
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 26:
        if (*skip == 0) {
            o->flags |= 0x20000;
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    default:
        h->pose = POSE_ENTRY(cmd);
        o->timer = o->saved;
        o->pc++;
        o->depth++;
        o->dc++;
        break;
    }
done:
    o->timer |= *fl;
}
