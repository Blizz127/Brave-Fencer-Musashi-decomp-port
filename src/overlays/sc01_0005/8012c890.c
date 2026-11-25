/* Owner W16 reconstruction of the scene's placement constructor. */
#include "saved_room_actor.h"
#include "scene_command.h"

typedef struct {
    u16 kind;
    u8 pad02[6];
    u16 x, y, z;
    u8 pad0E[4];
    s16 heading;
} ActorWork;

typedef void (*ActorSetup)(struct FileActor *);

extern u8 D_80185BC0[];
extern u16 D_801270C4[];
extern u16 D_801274E4[];
extern s32 D_8011DB08;
extern void func_80016714(s8 *buffer, s32 size);

struct FileActor *func_8012C890(struct FileActorPlacement *record, struct FileActor *actor, u32 owner)
{
    u32 *setup_table;
    ActorSetup setup;
    ActorWork *work;
    ActorWork *placed_work;
    u16 z;
    s16 heading;

    actor->type = record->type;
    actor->initial_x = actor->x = record->x;
    actor->initial_y = actor->y = record->y;
    z = record->z;
    actor->field04 = actor->field08 = actor->field0C = 0;
    actor->initial_z = actor->z = z;
    actor->subtype = record->subtype;
    actor->flags = record->flags & 0xf7ff;
    actor->fieldFC = record->field0E;
    __builtin_memcpy(actor->scene_fields, &record->user_data, sizeof(record->user_data));
    actor->shape = (u32)(SceneAddressInteger)D_80185BC0;
    actor->id = D_801270C4[0];
    D_801270C4[0]++;
    if ((s16)D_801270C4[0] == 0) {
        D_801270C4[0] = 1;
    }
    if (owner != 0) {
        actor->owner = owner;
        actor->placement = 0;
    } else {
        actor->owner = 0;
        actor->placement = (u32)(SceneAddressInteger)record;
    }
    record->flags |= 0x8000;
    actor->state = 0;
    D_801274E4[0] &= 0xfffe;
    setup_table = (u32 *)(SceneAddressInteger)(u32)D_8011DB08;
    setup = (ActorSetup)(SceneAddressInteger)setup_table[actor->type];
    setup(actor);
    if (D_801274E4[0] & 1) {
        work = (ActorWork *)(SceneAddressInteger)actor->work;
        if (work != 0) {
            switch (work->kind) {
            case 2:
                func_80016714((s8 *)work, 0x38);
                break;
            case 1:
                func_80016714((s8 *)work, 0x84);
                break;
            }
        }
        func_80016714((s8 *)actor, 0x10c);
        record->flags &= 0x7fff;
        return 0;
    }
    if (actor->owner == 0) {
        placed_work = (ActorWork *)(SceneAddressInteger)actor->work;
        if (placed_work == 0) {
            return actor;
        }
        if (placed_work->kind == 1) {
            heading = record->heading;
            if (heading != 0x7fff) {
                placed_work->heading = heading;
            }
        }
    }
    /* The original rereads the work address between coordinate stores. */
    if (actor->work != 0) {
        ((ActorWork *)(SceneAddressInteger)actor->work)->x = actor->x + actor->offset_x;
        ((ActorWork *)(SceneAddressInteger)actor->work)->y = actor->y + actor->offset_y;
        ((ActorWork *)(SceneAddressInteger)actor->work)->z = actor->z + actor->offset_z;
    }
    return actor;
}
