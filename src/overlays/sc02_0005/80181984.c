/* Update the six substates of the Steam Knight's third body action. */
#include "steam_knight_hit.h"

#include "steam_knight_state_three_services.h"

extern void func_80182998(SteamKnightPart *body);
extern void func_801838A8(SteamKnightPart *body);
extern void func_801846CC(SteamKnightPart *part);
extern void func_80184A34(SteamKnightPart *part);
extern s32 func_80185D84(SteamKnightPart *body);
extern void func_80186304(SteamKnightPart *body, int motion, int mode);
extern void func_80186F58(SteamKnightPart *body);
extern void func_80186F9C(SteamKnightPart *body, int joint);

#if defined(__mips__) && !defined(__LP64__)
extern u16 D_801E4BBC;
extern u8 D_80126B58[];
#define PLAYER D_80126B58
#define STAGE D_801E4BBC
#define LINK(key) ((SteamKnightPart *)(key))
#define LINK_STATE(key) (LINK(key)->state)
#define READ_HALF(p, off) (*(const u16 *)((const u8 *)(p) + (off)))
#define READ_WORD(p, off) (*(const u32 *)((const u8 *)(p) + (off)))
#define SET_HALF(p, off, value) (*(u16 *)((u8 *)(p) + (off)) = (value))
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
static u16 read_half(const void *object, unsigned offset)
{
    u16 value;
    memcpy(&value, (const u8 *)object + offset, sizeof value);
    return value;
}

static u32 read_word(const void *object, unsigned offset)
{
    u32 value;
    memcpy(&value, (const u8 *)object + offset, sizeof value);
    return value;
}

static void set_half(void *object, unsigned offset, u16 value)
{
    memcpy((u8 *)object + offset, &value, sizeof value);
}
#define PLAYER ((u8 *)bfm_tower_spawn_pointer(0x80126b58u))
#define STAGE read_half(bfm_tower_spawn_pointer(0x801e4bbcu), 0)
#define LINK(key) ((SteamKnightPart *)bfm_tower_spawn_pointer(key))
#define LINK_STATE(key) read_half(bfm_tower_spawn_pointer(key), 2)
#define READ_HALF read_half
#define READ_WORD read_word
#define SET_HALF set_half
#endif

#define SUBSTATE(body) READ_HALF(body, 0x34)
#define NEXT_SUBSTATE(body) SET_HALF(body, 0x34, SUBSTATE(body) + 1)
#if defined(__mips__) && !defined(__LP64__)
#define FRAME(body) (*(const s16 *)((const u8 *)(body) + 0x98))
#else
#define FRAME(body) ((s16)READ_HALF(body, 0x98))
#endif
#define ANIMATION(body) READ_WORD(body, 0x94)

void func_80181984(SteamKnightPart *body)
{
    u32 linked = body->secondary_address;
    u8 *player = PLAYER;
    u16 stage;

    if (body->phase_flags & 0x60000) {
        func_80182998(body);
        return;
    }
    switch (SUBSTATE(body)) {
    case 0:
        if (body->field_e0 >= 0)
            func_80185D84(body);
        if (ANIMATION(body) == 4 && FRAME(body) == 1)
            func_8002D4C8(1272, 0);
        if (func_8012BEE8(body)) {
            NEXT_SUBSTATE(body);
            func_80186304(body, 8, 2);
            body->phase_flags &= ~1u;
            func_80186F58(body);
        }
        break;
    case 1:
        if (ANIMATION(body) == 3 && FRAME(body) == 1) {
            NEXT_SUBSTATE(body);
            func_801846CC(LINK(linked));
        }
        break;
    case 2:
        if (!FRAME(body)) {
            func_80186F9C(body, 28);
            func_8002D4C8(1264, 0);
            NEXT_SUBSTATE(body);
        }
        /* Fall through. */
    case 3:
        if (LINK_STATE(linked) == 4) {
            if (body->phase_flags & 0x2000)
                body->timer = 32;
            else
                body->timer = 64;
            stage = STAGE;
            SET_HALF(body, 0x34, 4);
            if (!stage && !(body->phase_flags & 0x80) &&
                (u16)(READ_HALF(player, 6) + 191) < 143) {
                func_8012C588(94, 0);
                body->phase_flags |= 0x80;
            }
        }
        break;
    case 4:
        if (func_8012BEE8(body)) {
            NEXT_SUBSTATE(body);
            func_80186304(body, 9, 2);
            func_80184A34(LINK(linked));
        }
        break;
    case 5:
        if (!FRAME(body))
            func_801838A8(body);
        break;
    }
}
