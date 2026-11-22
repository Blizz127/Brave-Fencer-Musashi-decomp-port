#include "mine_controller_target.h"
#include "mine_target_draw.h"
#include "mine_pose_points.h"

extern u8 D_80126B58[];

#if BFM_MINE_DIRECT_GUEST_ADDRESS
extern s32 func_80148534(s32 from, s32 to);
extern void func_80165FA4(char *controller, int from, int to);
extern void func_80167DBC(void *controller, void *from, void *to);

#define word_at(p, offset) (*(u32 *)((p) + (offset)))
#define target_view(address) ((const u8 *)(address))
#define first_points(a, b) func_801484B0((u8 *)(a), (u8 *)(b))
#define second_points(a, b) func_80148534((s32)(a), (s32)(b))
#define transform_points(c, a, b) func_80165FA4((char *)(c), (int)(a), (int)(b))
#define first_draw func_80167DBC
#else
static u32 word_at(const u8 *p, unsigned offset)
{
    p += offset;
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}

static const u8 *target_view(u32 address)
{
    if (!address || (address & 3u) || address > 0xffffffdcu)
        return 0;
    return bfm_mine_controller_target_view(address, 36);
}

#define first_points bfm_mine_controller_first_points
#define second_points bfm_mine_controller_second_points
#define transform_points bfm_mine_controller_transform_points
#define first_draw bfm_mine_controller_first_draw
#endif

#if BFM_MINE_DIRECT_GUEST_ADDRESS
void func_80165E90(void)
#else
int bfm_mine_controller_update(void)
#endif
{
#if BFM_MINE_DIRECT_GUEST_ADDRESS
    s16 from[4], to[4];
#else
    s16 from[4] = {0}, to[4] = {0};
#endif
    u8 *controller = D_80126B58;
    const u8 *target;
    if ((word_at(D_80126B58, 0x44) & 0x10000400u) == 0x10000000u) {
        target = target_view(word_at(D_80126B58, 0x20));
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
        if (!target)
            return 0;
#endif
        if (!(word_at(target, 4) & 0x80000000u) && word_at(target, 0x20)) {
            first_points(from, to);
            transform_points(controller, from, to);
            first_draw(controller, from, to);
        }
    }
    if ((word_at(controller, 0x44) & 0x20020400u) == 0x20000000u) {
        target = target_view(word_at(controller, 0x20));
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
        if (!target)
            return 0;
#endif
        if (!(word_at(target, 4) & 0x80000000u) && word_at(target, 0x20)) {
            second_points(from, to);
            transform_points(controller, from, to);
            func_80167AE0(controller, from, to);
        }
    }
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    return 1;
#endif
}

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
void func_80165E90(void)
{
    (void)bfm_mine_controller_update();
}
#endif
