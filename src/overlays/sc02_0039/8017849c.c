/* Run active actor handlers and synchronize their child coordinates. */
#include "actor_position_updates.h"

#if defined(__mips__) && !defined(__LP64__)
#define BFM_ACTOR_ORIGINAL
#define ACTOR_HALF(p, offset) (*(u16 *)((u8 *)(p) + (offset)))
typedef struct {
    u8 preceding[4];
    s32 position[3];
    u8 before_previous[0x28];
    s32 previous[3];
} ActorPositions;
#else
static u16 actor_half(const Obj8014A2E4 *actor, unsigned offset)
{
    const u8 *p = (const u8 *)actor + offset;
    return (u16)((u16)p[0] | ((u16)p[1] << 8));
}

static void save_position(Obj8014A2E4 *actor)
{
    unsigned i;
    u8 *bytes = (u8 *)actor;
    for (i = 0; i < 12; i++) {
        bytes[0x38 + i] = bytes[4 + i];
    }
}

static int dispatch_actor(u32 table, Obj8014A2E4 *actor)
{
    u16 type = actor->active;
    u32 key;
    SceneActorUpdateHandler handler;
    if (!table || (table & 3u) || table > 0xffffffffu - 4u * type - 3u
        || !bfm_actor_update_handler_key(table, type, &key)) {
        bfm_actor_update_refused(BFM_ACTOR_UPDATE_BAD_TABLE, table, type);
        return 0;
    }
    handler = (!key || (key & 3u)) ? (SceneActorUpdateHandler)0 : bfm_actor_update_handler(key);
    if (!handler) {
        bfm_actor_update_refused(BFM_ACTOR_UPDATE_BAD_HANDLER, key, type);
        return 0;
    }
    handler(actor);
    return 1;
}

static int synchronize_child(Obj8014A2E4 *actor)
{
    unsigned axis;
    if (!actor->probe_object_address) {
        return 1;
    }
    for (axis = 0; axis < 3; axis++) {
        unsigned offset = 8 + axis * 2;
        u16 high = actor_half(actor, 6 + axis * 4);
        u16 adjust = actor_half(actor, 0x50 + axis * 2);
        u16 value = (u16)(high + adjust);
        u32 key = actor->probe_object_address;
        u8 *field = 0;
        if (key && !(key & 1u) && key <= 0xffffffffu - offset - 1u) {
            field = bfm_actor_update_child_span(key, offset, 2);
        }
        if (!field) {
            bfm_actor_update_refused(BFM_ACTOR_UPDATE_BAD_CHILD, key, offset);
            return 0;
        }
        field[0] = (u8)value;
        field[1] = (u8)(value >> 8);
    }
    return 1;
}
#define ACTOR_HALF(p, offset) actor_half((p), (offset))
#endif

void func_8017849C(void)
{
    Obj8014A2E4 *actor;
#ifdef BFM_ACTOR_ORIGINAL
    u32 *table;
#else
    u32 table;
#endif
    s32 i;

    actor = D_801202A0;
    func_801787D4();
    func_8012A908();
    for (i = 0; i < 96; i++, actor++) {
        if (!actor->active) {
            continue;
        }
        if (D_801270C0 != 1 && !(ACTOR_HALF(actor, 0x72) & 0x1000)) {
            continue;
        }
        func_8017869C(actor);
#ifdef BFM_ACTOR_ORIGINAL
        ((ActorPositions *)actor)->previous[0] = ((ActorPositions *)actor)->position[0];
        ((ActorPositions *)actor)->previous[1] = ((ActorPositions *)actor)->position[1];
        ((ActorPositions *)actor)->previous[2] = ((ActorPositions *)actor)->position[2];
        table = (u32 *)D_8011DB08;
#else
        save_position(actor);
        table = D_8011DB08;
#endif
        if (((u8 *)actor)[0xc0] && ((u8 *)actor)[0xc1]) {
            func_80130D0C(actor);
        } else {
#ifdef BFM_ACTOR_ORIGINAL
            ((SceneActorUpdateHandler)table[actor->active])(actor);
#else
            if (!dispatch_actor(table, actor)) {
                return;
            }
#endif
        }
#ifdef BFM_ACTOR_ORIGINAL
        if (actor->probe_object_address) {
            ACTOR_HALF(actor->probe_object_address, 8) = ACTOR_HALF(actor, 6) + ACTOR_HALF(actor, 0x50);
            ACTOR_HALF(actor->probe_object_address, 10) = ACTOR_HALF(actor, 10) + ACTOR_HALF(actor, 0x52);
            ACTOR_HALF(actor->probe_object_address, 12) = ACTOR_HALF(actor, 14) + ACTOR_HALF(actor, 0x54);
        }
#else
        if (!synchronize_child(actor)) {
            return;
        }
#endif
    }
    if (D_801270C0 == 1) {
        func_8013E370();
    }
}
