#include <assert.h>
#include <string.h>
#include "steam_knight_hit.h"

void func_80186600(SteamKnightPart *);
void func_80186DE8(SteamKnightPart *);

s16 D_801E43B8;
s32 D_801E43A0;
static SteamKnightPart body, link_actor;
static u8 body_model[32], first_model[32], next_model[32];
static int joints[8], joint_count, sound_count;
static u16 last_sound;
static int heading_queries, angle_queries, change_model, change_mask;

static void put_half(void *base, unsigned offset, s16 value)
{
    memcpy((u8 *)base + offset, &value, sizeof value);
}
static s16 get_half(const void *base, unsigned offset)
{
    s16 value;
    memcpy(&value, (const u8 *)base + offset, sizeof value);
    return value;
}
void *bfm_tower_spawn_pointer(u32 address)
{
    switch (address) {
    case 1: return &link_actor;
    case 2: return body_model;
    case 3: return first_model;
    case 4: return next_model;
    default: assert(0); return 0;
    }
}
void func_80186F9C(SteamKnightPart *actor, int joint)
{
    assert(actor == &body && joint_count < 8);
    joints[joint_count++] = joint;
    if (change_mask && joint == 5) D_801E43A0 |= 0x100;
    if (change_mask && joint == 15) D_801E43A0 |= 0x1000;
}
void func_8002D4C8(u16 command, u16 argument)
{
    assert(argument == 0);
    last_sound = command;
    sound_count++;
}
s32 func_8012B864(SteamKnightPart *actor)
{
    assert(actor == &body);
    heading_queries++;
    return -1700;
}
s32 func_8012B608(s32 current, s32 target, s32 step)
{
    assert(current == -23 && target == -1700 && step == 8);
    angle_queries++;
    if (change_model) link_actor.model_address = 4;
    return -31;
}
static void reset(void)
{
    memset(&body, 0, sizeof body);
    memset(&link_actor, 0, sizeof link_actor);
    body.linked_address = 1;
    body.model_address = 2;
    link_actor.model_address = 3;
    D_801E43B8 = 120;
    D_801E43A0 = 0;
    joint_count = sound_count = heading_queries = angle_queries = 0;
    change_model = change_mask = 0;
}
int main(void)
{
    reset();
    body.linked_address = 0;
    func_80186600(&body);
    assert(D_801E43B8 == 0 && heading_queries == 0);

    reset();
    body.phase_flags = 1;
    put_half(first_model, 0x12, -52);
    put_half(body_model, 0x12, 29);
    put_half(next_model, 0x12, 37);
    change_model = 1;
    func_80186600(&body);
    assert(heading_queries == 1 && angle_queries == 1 && D_801E43B8 == -31);
    assert(get_half(first_model, 0x12) == -52 && get_half(next_model, 0x12) == 6);

    reset();
    body.phase_flags = 0x201;
    put_half(&body, 0xe6, 64);
    func_80186600(&body);
    assert(joint_count == 2 && joints[0] == 22 && joints[1] == 25);
    assert(last_sound == 1264 && get_half(&body, 0xe6) == 65);
    put_half(&body, 0xe6, 84);
    func_80186600(&body);
    assert(body.phase_flags == 0x301 && last_sound == 1260 && sound_count == 2);
    assert(get_half(&body, 0xe6) == 85);
    put_half(&body, 0xe4, 1);
    put_half(first_model, 0x12, 32640);
    func_80186600(&body);
    assert(D_801E43B8 == 256 && get_half(first_model, 0x12) == -32640);
    assert(get_half(&body, 0xe4) == 0);
    func_80186600(&body);
    assert(D_801E43B8 == 0 && get_half(&body, 0xe6) == 0 && body.phase_flags == 1);

    reset();
    body.field_fc = 1;
    change_mask = 1;
    func_80186DE8(&body);
    assert(body.field_fc == 12 && body.phase_flags == 0x40);
    assert(joint_count == 2 && joints[0] == 5 && joints[1] == 15);
    assert(sound_count == 1 && last_sound == 1264);
    body.field_fc = 1;
    func_80186DE8(&body);
    assert(body.field_fc == 8 && body.phase_flags == 0);
    D_801E43A0 = 0x1540;
    func_80186DE8(&body);
    assert(body.field_fc == 0 && joint_count == 2 && sound_count == 1);
    D_801E43A0 = 0;
    body.field_fc = 0x8000;
    func_80186DE8(&body);
    assert(body.field_fc == 0x7fff && sound_count == 1);
    return 0;
}
