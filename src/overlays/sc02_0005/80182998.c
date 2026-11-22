/* Enter the Steam Knight body state for the armor phase flags. */
#include "steam_knight_hit.h"

void func_8012B23C(s32 *actor);
void func_80186F58(SteamKnightPart *body);
void func_80184C20(SteamKnightPart *part);
void func_8018AAAC(void);
void func_8012AD44(u8 *actor, u16 state);
void func_80186304(SteamKnightPart *body, int motion, int mode);

#if defined(__mips__) && !defined(__LP64__)
#define BODY_ACTOR(address) ((SteamKnightPart *)(address))
#define SET_SUBSTATE(body, value) (*(u16 *)((u8 *)(body) + 0x34) = (value))
#define CLEAR_MODEL_Y(body) (*(s16 *)(BODY_ACTOR((body)->linked_address)->model_address + 0x12) = 0)
#else
#include <string.h>
void *bfm_tower_spawn_pointer(u32 address);
#define BODY_ACTOR(address) ((SteamKnightPart *)bfm_tower_spawn_pointer(address))

static void set_substate(SteamKnightPart *body, u16 value)
{
    memcpy((u8 *)body + 0x34, &value, sizeof value);
}

static void clear_model_y(SteamKnightPart *body)
{
    const u8 *linked = bfm_tower_spawn_pointer(body->linked_address);
    u32 model_address;
    u8 *model;
    u16 zero = 0;
    memcpy(&model_address, linked + 0x20, sizeof model_address);
    model = bfm_tower_spawn_pointer(model_address);
    memcpy(model + 0x12, &zero, sizeof zero);
}
#define SET_SUBSTATE(body, value) set_substate(body, value)
#define CLEAR_MODEL_Y(body) clear_model_y(body)
#endif

void func_80182998(SteamKnightPart *body)
{
    func_8012B23C((s32 *)body);
    body->phase_flags &= ~1u;
    func_80186F58(body);
    CLEAR_MODEL_Y(body);
    func_80184C20(BODY_ACTOR(body->secondary_address));
    func_8018AAAC();
    func_8012AD44((u8 *)body, 11);
    if (body->phase_flags & 0x1000)
        body->break_cause = (s16)body->timer;
    if ((body->phase_flags & 0x60000) == 0x60000) {
        func_80186304(body, 0x19, 2);
        SET_SUBSTATE(body, 2);
    } else if (body->phase_flags & 0x20000) {
        func_80186304(body, 0x15, 2);
        SET_SUBSTATE(body, 0);
    } else {
        func_80186304(body, 0x17, 2);
        SET_SUBSTATE(body, 1);
    }
}
