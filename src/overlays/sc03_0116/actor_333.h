#ifndef BFM_SC03_0116_ACTOR_333_H
#define BFM_SC03_0116_ACTOR_333_H

#include "psx_types.h"

typedef struct {
    s16 unk0, unk2;
    u8 pad4[0x1c];
    s32 unk20, unk24;
} Actor333ListPrefix;

typedef union {
    Actor333ListPrefix prefix;
    struct {
        u8 before_flags[4];
        u32 flags;
        u8 before_table[0x18];
        u32 table;
        u8 before_options[8];
        u16 options;
        u8 before_vector[0x52];
        u32 vector;
    } fields;
} Actor333Model;

typedef struct {
    u16 type, phase;
    u8 before_model[0x1c];
#if defined(__mips__) && !defined(__LP64__)
    Actor333Model *model;
#else
    u32 model_address;
#endif
    u8 before_shape[0x34];
    u32 shape;
    u16 contact_flags;
    u8 before_vector[0x82];
    s16 vector[4];
    u8 tail[0x24];
} Actor333;

s32 func_80181FA8(Actor333 *actor);
s32 func_80029178(u32 event);
void *func_8012C1B8(void);
s32 func_8012CAE4(u8 *actor);
typedef struct {
    s16 value;
} Actor333Part;

extern Actor333Part D_801864C8, D_801864D4;

#if defined(__mips__) && !defined(__LP64__)
extern s32 D_80194EE0[];
extern const u8 D_801864E0[];
void func_8001C810(Actor333ListPrefix *model, s32 *list);
#else
u32 bfm_room_333_model_address(Actor333Model *model);
Actor333Model *bfm_room_333_attached_model(const Actor333 *actor);
void bfm_room_333_bind_model(Actor333Model *model);
extern const u32 bfm_room_333_shape_address;
extern const u32 bfm_room_333_table_address;
u32 bfm_room_333_vector_address(const Actor333 *actor);
#endif

#endif
