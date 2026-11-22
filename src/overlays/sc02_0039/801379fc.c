#include "mine_actor_fade.h"

extern MineVisualActor D_801269F0[];
extern u32 D_80127524;
extern s32 D_80127548;

#if BFM_MINE_DIRECT_GUEST_ADDRESS
extern void func_80138BE0(MineVisualActor *actor);
extern void func_80137BD8(MineVisualActor *actor);
#define active(p) ((p)->active)
#define active_slot(i) D_801269F0[i].active
#define kind_slot(i) D_801269F0[i].kind
#define flags_slot(i) D_801269F0[i].flags
#define focus_actor(addr) ((MineVisualActor *)(addr))
#define same_as_focus(i) (&D_801269F0[i] == (MineVisualActor *)D_80127524)
#define update_actor(addr, p) func_80138BE0(p)
#define draw_slot(i) func_80137BD8(&D_801269F0[i])
#else
static u32 read_word(const u8 *p)
{
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}

#define active(p) read_word((p)->bytes)
#define active_slot(i) active(&D_801269F0[i])
#define kind_slot(i) (D_801269F0[i].bytes[4] | (unsigned)D_801269F0[i].bytes[5] << 8)
#define flags_slot(i) read_word(D_801269F0[i].bytes + 8)
#define slot_address(i) (BFM_MINE_VISUAL_ACTORS_BASE + 0x4cu * (u32)(i))
#define same_as_focus(i) (slot_address(i) == D_80127524)
#define update_actor(addr, p) bfm_mine_update_visual_actor(addr, p)
#define draw_slot(i) bfm_mine_draw_visual_actor(slot_address(i), &D_801269F0[i])

static MineVisualActor *focus_actor(u32 address)
{
    unsigned i;
    if ((address & 3u) || address > 0xffffffb4u)
        return 0;
    for (i = 0; i < 3; ++i)
        if (address == slot_address(i))
            return &D_801269F0[i];
    return bfm_mine_external_visual_actor(address);
}
#endif

#if BFM_MINE_DIRECT_GUEST_ADDRESS
void func_801379FC(void)
#else
int bfm_mine_actor_fade_update(void)
#endif
{
    s32 i;
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    MineVisualActor *focus;
#endif
    D_80127548 = 0x24;
#if BFM_MINE_DIRECT_GUEST_ADDRESS
    if (D_80127524 && active(focus_actor(D_80127524))) {
        update_actor(D_80127524, focus_actor(D_80127524));
#else
    focus = D_80127524 ? focus_actor(D_80127524) : 0;
    if (D_80127524 && !focus)
        return 0;
    if (focus && active(focus)) {
        update_actor(D_80127524, focus);
#endif
        for (i = 0; i < 3; ++i) {
            if (!same_as_focus(i) && active_slot(i))
                update_actor(slot_address(i), &D_801269F0[i]);
        }
    } else {
        for (i = 0; i < 3; ++i) {
            if (active_slot(i))
                update_actor(slot_address(i), &D_801269F0[i]);
        }
    }
    for (i = 0; i < 3; ++i) {
        if (kind_slot(i) && active_slot(i)
                && (flags_slot(i) & 0x1000u)
                && !(flags_slot(i) & 0x2000u))
            draw_slot(i);
    }
    func_8013A380();
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    return 1;
#endif
}

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
void func_801379FC(void)
{
    (void)bfm_mine_actor_fade_update();
}
#endif
