/* Choose the next scripted action for the Steam Knight body. */
#include "steam_knight_hit.h"

extern s32 func_801860F4(SteamKnightPart *body, s16 action);
extern s32 func_8018AD20(SteamKnightPart *body);
extern s32 func_8018AC48(SteamKnightPart *body);
extern s16 func_80186144(SteamKnightPart *body, int action);

#if defined(__mips__) && !defined(__LP64__)
typedef struct {
    s16 delay;
    u16 action;
} SteamKnightAction;
extern SteamKnightAction *D_801959E0[];
extern u16 D_801E4BBC;
extern s32 D_801E43A0;
#define ACTION_TABLE SteamKnightAction *
#define MODE D_801E4BBC
#define TABLE_FOR(mode) D_801959E0[mode]
#define GLOBAL_FLAGS D_801E43A0
#define ACTION(table, index) (table)[index].action
#define DELAY(table, index) (table)[index].delay
#define INDEX(body) (*(s16 *)(body)->pad102)
#define PUT_INDEX(body, index) (INDEX(body) = (index))
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
static u16 read_half(const void *object)
{
    u16 value;
    memcpy(&value, object, sizeof value);
    return value;
}
static u32 read_word(u32 address)
{
    u32 value;
    memcpy(&value, bfm_tower_spawn_pointer(address), sizeof value);
    return value;
}
static s16 set_index(SteamKnightPart *body, s16 index)
{
    memcpy(body->pad102, &index, sizeof index);
    return index;
}
#define ACTION_TABLE u32
#define MODE read_half(bfm_tower_spawn_pointer(0x801e4bbcu))
#define TABLE_FOR(mode) read_word(0x801959e0u + 4u * (mode))
#define GLOBAL_FLAGS read_word(0x801e43a0u)
#define ACTION(table, index) read_half(bfm_tower_spawn_pointer((table) + 4u * (u32)(index) + 2u))
#define DELAY(table, index) ((s16) read_half(bfm_tower_spawn_pointer((table) + 4u * (u32)(index))))
#define INDEX(body) ((s16)read_half((body)->pad102))
#define PUT_INDEX(body, index) set_index(body,index)
#endif

s16 func_80185E80(SteamKnightPart *body)
{
    ACTION_TABLE table = TABLE_FOR(MODE);
    s16 action;
    s16 tagged;
    s16 delay;
    if (body->break_delay != 0 || (body->phase_flags & 0x200))
        return 0;

    action = ACTION(table, INDEX(body));
    tagged = action & 0x100;
    action &= 0xff;
    switch (action) {
    case 2:
        if (!func_801860F4(body, action) || (GLOBAL_FLAGS & 0x2000))
            action = 0;
        break;
    case 3:
        body->field_e0 = func_8018AD20(body);
        if (body->field_e0 < 0)
            action = 0;
        break;
    case 4:
        body->field_e0 = func_8018AC48(body);
        if (body->field_e0 < 0)
            action = 1;
        break;
    case 9:
        body->field_e0 = func_80186144(body, 9);
        if (body->field_e0 < 0)
            action = 1;
        break;
    case 5:
        if (body->phase_flags & 0x408)
            action = 0;
        break;
    case 6:
    case 7:
        if (!(body->phase_flags & 0x800) && body->hp <= 128)
            action = 0;
        break;
    case 8:
        if (body->phase_flags & 0x800) {
            action = 0;
        } else if (!(body->phase_flags & 0x400)) {
            if ((body->phase_flags & 10) == 2)
                body->break_count++;
            goto no_action;
        } else {
            body->field_e0 = func_80186144(body, 8);
            if (body->field_e0 < 0)
                action = 6;
        }
        break;
    }

    delay = DELAY(table, PUT_INDEX(body, INDEX(body) + 1));
    if (delay == -1) {
        u16 mode = MODE;
        PUT_INDEX(body, 0);
        body->break_delay = DELAY(TABLE_FOR(mode), 0);
    } else {
        body->break_delay = delay;
    }
    if (tagged && (body->phase_flags & 10) == 2)
        body->break_count++;
    return action;
no_action:
    return 0;
}
