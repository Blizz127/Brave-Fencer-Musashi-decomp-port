#include "steam_knight_hit.h"

extern void func_80182810(SteamKnightPart *body);
extern void func_801828D8(SteamKnightPart *body);
extern s16 func_80185E80(SteamKnightPart *body);
extern void func_80182740(SteamKnightPart *body);
extern void func_801827A8(SteamKnightPart *body);
extern void func_8012B23C(s32 *body);
extern void func_80186304(SteamKnightPart *body, int motion, int mode);
extern void func_8012AD44(u8 *body, u16 state);
extern void func_80186F14(SteamKnightPart *body);

#if defined(__mips__) && !defined(__LP64__)
extern void (*D_801962F8[])(SteamKnightPart *);
extern u16 D_801E4BBC;
extern s16 D_80126CB4;
extern s16 D_80195AD2;
typedef struct { s16 right, left; } BodyBounds;
extern BodyBounds D_80195AD0[];
extern u8 D_80195B2C[];
#define BODY_X(body) (*(s16 *)((u8 *)(body) + 6))
#define PLAYER_X D_80126CB4
#define MODE D_801E4BBC
#define LEFT_BOUND D_80195AD2
#define RIGHT_BOUND(mode) D_80195AD0[(mode)].right
#define DISPATCH(index,body) D_801962F8[(index)]((body))
#define ACTIVE_DISPLAY ((u32)D_80195B2C | 0x40000000u)
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
extern void bfm_tower_spawn_setup(u32 target, void *actor);
static s16 read_short(const void *memory)
{
    s16 value;
    memcpy(&value, memory, sizeof value);
    return value;
}
static void dispatch(s16 index, SteamKnightPart *body)
{
    u32 target;
    memcpy(&target, bfm_tower_spawn_pointer(0x801962F8u + 4u * (u32)(s32)index), sizeof target);
    bfm_tower_spawn_setup(target, body);
}
#define BODY_X(body) read_short((u8 *)(body) + 6)
#define PLAYER_X read_short(bfm_tower_spawn_pointer(0x80126CB4u))
#define MODE ((u16)read_short(bfm_tower_spawn_pointer(0x801E4BBCu)))
#define LEFT_BOUND read_short(bfm_tower_spawn_pointer(0x80195AD2u))
#define RIGHT_BOUND(mode) read_short(bfm_tower_spawn_pointer(0x80195AD0u + 4u * (mode)))
#define DISPATCH(index,body) dispatch((index),(body))
#define ACTIVE_DISPLAY 0xC0195B2Cu
#endif

void func_801838A8(SteamKnightPart *body)
{
    s16 decision;
    s16 distance;
    s16 count;

    if (!(body->phase_flags & 8) && (count = (s16)body->break_count) != 0) {
        body->break_count = count - 1;
        func_80182810(body);
        if (body->phase_flags & 0x2000)
            body->timer = 24;
        else
            body->timer = 48;
        func_801828D8(body);
        body->phase_flags &= ~0x80u;
        return;
    }
    decision = func_80185E80(body);
    if (decision) {
        DISPATCH(decision, body);
        return;
    }
    if (MODE == 0) {
        distance = BODY_X(body) - PLAYER_X;
        body->motion_x = 0;
        if (distance < -128 && BODY_X(body) < LEFT_BOUND) {
            func_80182740(body);
            if (body->phase_flags & 0x2000)
                body->timer = 24;
            else
                body->timer = 48;
            func_801828D8(body);
            return;
        }
        if (distance > 128 && BODY_X(body) > RIGHT_BOUND(MODE)) {
            func_801827A8(body);
            if (body->phase_flags & 0x2000)
                body->timer = 24;
            else
                body->timer = 48;
            func_801828D8(body);
            return;
        }
    }
    if (body->state == 1) {
        if (body->phase_flags & 0x2000)
            body->timer = 16;
        else
            body->timer = 32;
        return;
    }
    func_8012B23C((s32 *)body);
    body->phase_flags |= 1;
    func_80186304(body, 0, 0);
    if (body->phase_flags & 0x2000)
        body->timer = 16;
    else
        body->timer = 32;
    body->display_record = ACTIVE_DISPLAY;
    body->flags = 0;
    body->field_ae = -1;
    func_8012AD44((u8 *)body, 1);
    func_80186F14(body);
}
