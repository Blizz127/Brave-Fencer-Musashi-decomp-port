#include "steam_knight_armor_setup.h"

typedef struct {
    u8 bytes[52];
} ArmorTemplate;

extern int func_8012C354(u8 *actor, u8 *record);
extern void *func_8012C1B8(void);

#if defined(__mips__) && !defined(__LP64__)
extern u16 D_80195B4C[], D_80195B68[];
extern ArmorTemplate D_80195D74[];
extern u8 D_80196490[];
extern s32 func_8012CAE4(u8 *actor);
#include "steam_knight_model_binding.h"
extern s32 func_8012AD50(u8 *actor);
#define OWNER_LINK(key) (((SteamKnightPart *)(key))->linked_address)
#define JOINT(table, index) ((table)[index])
#define ADDRESS(ptr) ((u32)(ptr))
#define ARMOR_LOW D_80195B4C
#define ARMOR_HIGH D_80195B68
#define RECORDS D_80195D74
#define REACTIONS ((u32)D_80196490)
#define STORE_WORD(part, offset, value) (*(u32 *)((u8 *)(part) + (offset)) = (value))
#define READ_WORD(part, offset) (*(u32 *)((u8 *)(part) + (offset)))
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 key);
extern u32 bfm_tower_spawn_address(const void *p);
#define func_8012CAE4 bfm_tower_armor_dispose
#define func_8001C2C4 bfm_tower_armor_bind_model
#define func_8012AD50 bfm_tower_armor_advance
#define OWNER_LINK(key) read_word(bfm_tower_spawn_pointer((key) + 204u), 0)
#define JOINT(table, index) read_half((const u8 *)(table) + 2u * (index))
#define ADDRESS(ptr) ((ptr) ? bfm_tower_spawn_address(ptr) : 0u)
#define ARMOR_LOW ((u16 *)bfm_tower_spawn_pointer(0x80195B4Cu))
#define ARMOR_HIGH ((u16 *)bfm_tower_spawn_pointer(0x80195B68u))
#define RECORDS ((ArmorTemplate *)bfm_tower_spawn_pointer(0x80195D74u))
#define REACTIONS 0x80196490u
static u16 read_half(const u8 *bytes)
{
    return (u16)(bytes[0] | ((u16)bytes[1] << 8));
}
static u32 read_word(const void *part, u32 offset)
{
    u32 value; memcpy(&value, (const u8 *)part + offset, sizeof value); return value;
}
static void store_word(void *part, u32 offset, u32 value)
{
    memcpy((u8 *)part + offset, &value, sizeof value);
}
#define STORE_WORD(part, offset, value) store_word(part, offset, value)
#define READ_WORD(part, offset) read_word(part, offset)
#endif

void func_80184EC0(SteamKnightPart *part)
{
    u16 index = part->variant;
    u16 *joints;
    u16 joint;
    u32 owner;
    void *model;

    if (index >= 19) {
        func_8012CAE4((u8 *)part);
        return;
    }
    if ((s16)index >= 13) {
        joints = ARMOR_HIGH;
        index -= 13;
    } else {
        joints = ARMOR_LOW;
    }
    if (!func_8012C354((u8 *)part, RECORDS[part->variant].bytes)) return;

    /* Enable the hit-event table and reset local coordinates and reaction state. */
    ((u8 *)part)[192] = 1;
    STORE_WORD(part, 188, REACTIONS);
    STORE_WORD(part, 12, 0);
    STORE_WORD(part, 8, 0);
    STORE_WORD(part, 4, 0);
    STORE_WORD(part, 180, 0);
    part->reaction = 0;
    ((u8 *)part)[117] = 8;
    part->field_dc = 0;
    part->break_delay = 0;
    STORE_WORD(part, 196, READ_WORD(part, 196) | 2);
    model = func_8012C1B8();
    part->model_address = ADDRESS(model);
    if (!model) {
        func_8012CAE4((u8 *)part);
        return;
    }
    func_8001C2C4(model);
    part->linked_address = part->body_address;
    joint = JOINT(joints, index);
    /* High joint indices attach through the body's related actor. */
    if (joint >= 22) {
        part->field_fc = joint - 22;
        owner = OWNER_LINK(part->body_address);
        part->break_count = 1;
        part->body_address = owner;
    } else {
        part->field_fc = joint;
        part->break_count = 0;
        if (joint == 4 || joint == 9 || joint == 14 || joint == 19)
            part->break_count = 2;
    }
    func_8012AD50((u8 *)part);
}
