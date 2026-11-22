#include "steam_knight_actor84.h"

extern void func_8012AD80(SteamKnightPart *actor);
extern s32 func_8012C218(u8 *actor);

#if defined(__mips__) && !defined(__LP64__)
typedef struct {
    u8 pad[16];
    u16 x, y;
} ModelRotation;
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);

static void add_rotation(u32 model, unsigned offset, u16 increment)
{
    u16 value;
    u8 *field = (u8 *)bfm_tower_spawn_pointer(model) + offset;
    memcpy(&value, field, sizeof value);
    value += increment;
    memcpy(field, &value, sizeof value);
}
#endif

void func_8018BAE4(SteamKnightPart *actor)
{
#if defined(__mips__) && !defined(__LP64__)
    ((ModelRotation *)actor->model_address)->y += actor->field_fc;
    ((ModelRotation *)actor->model_address)->x += actor->break_count;
#else
    /* The first halfword store can change the next model address. */
    add_rotation(actor->model_address, 18, actor->field_fc);
    add_rotation(actor->model_address, 16, actor->break_count);
#endif
    func_8012AD80(actor);
    if (actor->position_y >= -2047) func_8012C218((u8 *)actor);
}
