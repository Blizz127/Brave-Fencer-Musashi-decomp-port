#ifndef BFM_SAVED_ROOM_ACTOR_H
#define BFM_SAVED_ROOM_ACTOR_H

#include "psx_types.h"

struct FileActorPlacement {
    u16 x, y, z;
    u16 type;
    u16 subtype;
    u16 flags;
    s16 heading;
    u16 field0E;
    u32 user_data;
};

struct FileActor {
    u16 type, state;
    u16 field04, x, field08, y, field0C, z;
    u8 reserved_10[0x1c - 0x10];
    u32 delay;
    u32 work;
    u8 reserved_24[0x36 - 0x24];
    u16 id;
    u8 reserved_38[0x50 - 0x38];
    u16 offset_x, offset_y, offset_z;
    u8 reserved_56[0x64 - 0x56];
    u32 owner, placement;
    u8 reserved_6c[4];
    s16 subtype;
    u16 flags;
    u8 reserved_74[4];
    u32 shape;
    u8 reserved_7c[0x88 - 0x7c];
    u16 initial_x, initial_y, initial_z;
    u8 reserved_8e[0xdc - 0x8e];
    s16 scene_fields[2];
    u8 reserved_e0[0xfc - 0xe0];
    u16 fieldFC;
    u8 reserved_fe[0x106 - 0xfe];
    s16 field106; /* Bottom fade level. */
    s16 field108; /* Top fade level. */
    s16 field10A;
};

struct FileActor *func_8012C750(struct FileActorPlacement *record);
struct FileActor *func_8012C890(struct FileActorPlacement *record,
                               struct FileActor *actor, u32 owner);

void func_8017FD50(struct FileActor *actor);
void func_8017FF74(struct FileActor *actor);
void func_80180010(struct FileActor *actor);
void func_801803AC(struct FileActor *actor);
u32 func_80180808(struct FileActor *actor);

#endif
