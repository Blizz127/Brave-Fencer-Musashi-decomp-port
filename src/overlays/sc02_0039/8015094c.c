#include "mine_focus_step.h"
#include "mine_guest_abi.h"

typedef struct {
    s32 position[3];
    u32 reserved0c[9];
    MineFocusParameters parameters;
    u32 reserved40[4];
} FocusQuery;

#if BFM_MINE_DIRECT_GUEST_ADDRESS
extern s32 D_80126990, D_80126994;
#define focus_x D_80126990
#define focus_y D_80126994
#define coord(actor, offset) (*(s16 *)((u8 *)(actor) + (offset)))
#define put_coord(actor, offset, value) (coord(actor, offset) = (value))
#define word_delta(goal, position) ((goal) - (position))
#define word_negate(value) (-(value))
#else
#define focus_x D_80126948.focus.x
#define focus_y D_80126948.focus.y
static s16 coord(const Obj801508F8 *actor, unsigned offset)
{
    const u8 *p = (const u8 *)actor + offset;
    u16 bits = (u16)(p[0] | (u16)p[1] << 8);
    return (s16)((s32)(bits ^ 0x8000u) - 0x8000);
}

static void put_coord(Obj801508F8 *actor, unsigned offset, s32 value)
{
    u8 *p = (u8 *)actor + offset;
    p[0] = (u8)value;
    p[1] = (u8)((u16)value >> 8);
}

static s32 signed_word(u32 value)
{
    return (s32)((long long)(value ^ 0x80000000u) - 0x80000000LL);
}

#define word_delta(goal, position) signed_word((u32)(goal) - (u32)(position))
#define word_negate(value) signed_word(0u - (u32)(value))
#endif

s32 func_8015094C(Obj801508F8 *actor)
{
    FocusQuery query;
    s32 delta, magnitude;
    FacingActor *focus = &D_80126948;
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    query = (FocusQuery){0};
#endif
    query.parameters = D_8019F170;
    query.position[0] = coord(actor, 6);
    query.position[1] = focus_y;
    query.position[2] = coord(actor, 14);
    if (!func_80021174(D_80180D74, query.position)) {
        delta = word_delta(focus_x, coord(actor, 6));
        magnitude = delta;
        if (delta < 0)
            magnitude = word_negate(delta);
        if (magnitude >= 5) {
            if (delta >= 0)
                put_coord(actor, 6, coord(actor, 6) + 4);
            else
                put_coord(actor, 6, coord(actor, 6) - 4);
        }
        delta = word_delta(focus->focus.z, coord(actor, 14));
        magnitude = delta;
        if (delta < 0)
            magnitude = word_negate(delta);
        if (magnitude >= 5) {
            if (delta >= 0)
                put_coord(actor, 14, coord(actor, 14) + 4);
            else
                put_coord(actor, 14, coord(actor, 14) - 4);
        }
        return 1;
    }
    return 0;
}
