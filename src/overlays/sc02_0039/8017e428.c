/* Set up the room rotor, its optional accessory and the linked platform actors. */
#include "mine_rotor_init.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef struct {
    u32 reserved, flags;
    u16 x, y, z;
    u8 gap[0x1a];
    u32 scale;
} Model;
typedef struct {
    s16 unk0, unk2;
    u8 pad4[0x1c];
    s32 unk20, unk24;
} ListModel;
typedef struct {
    u8 pad0[2];
    u16 cnt;
    u8 pad4[0x30];
    u16 clr;
} StateActor;
extern void *func_8012C1B8(void);
extern s32 func_8012CAE4(u8 *actor);
extern void func_8001C810(ListModel *model, s32 *list);
extern void func_8001C214(ListModel *model, s32 *list);
extern void func_8012AD50(StateActor *actor);
extern s32 D_8019CA6C[], D_8019DEE0[];
void func_8017E428(Obj8014A2E4 *actor)
{
    Model *accessory;
    actor->probe_object_address = (u32)func_8012C1B8();
    if (!actor->probe_object_address) {
        func_8012CAE4((u8 *)actor);
        return;
    }
    func_8001C810((ListModel *)actor->probe_object_address, D_8019CA6C);
    ((Model *)actor->probe_object_address)->flags |= 0x8040;
    ((Model *)actor->probe_object_address)->scale = 0x01800180;
    accessory = (Model *)func_8012C1B8();
    if (accessory) {
        func_8001C214((ListModel *)accessory, D_8019DEE0);
        accessory->x = *(u16 *)(actor->pad002 + 4);
        accessory->y = *(u16 *)(actor->pad002 + 8) + 0xc0;
        accessory->z = *(u16 *)(actor->pad002 + 12) - 0xb0;
    }
    func_8017DE00(actor);
    func_8012AD50((StateActor *)actor);
}
#else
static u16 read_half(const u8 *p)
{
    return (u16)(p[0] | (u16)p[1] << 8);
}
static u32 read_word(const u8 *p)
{
    return read_half(p) | (u32)read_half(p + 2) << 16;
}
static void write_half(u8 *p, u16 value)
{
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}
static void write_word(u8 *p, u32 value)
{
    write_half(p, (u16)value);
    write_half(p + 2, (u16)(value >> 16));
}

int mine_rotor_initialize(Obj8014A2E4 *actor, const MineRotorInit *services)
{
    if (!actor || !services || !services->allocate_model)
        return 0;

    u8 *physical = (u8 *)actor;
    u32 key = services->allocate_model(services->context);
    write_word(physical + 0x20, key);
    if (!key) {
        if (!services->remove_actor)
            return 0;
        services->remove_actor(services->context, actor);
        return 1;
    }
    if (!services->configure)
        return 0;
    services->configure(services->context, key, MINE_ROTOR_PRIMARY_MODEL);
    if (!services->model)
        return 0;
    MineRotorModelFields *primary = services->model(services->context, read_word(physical + 0x20));
    if (!primary)
        return 0;
    write_word(primary->bytes + 4, read_word(primary->bytes + 4) | 0x8040);
    primary = services->model(services->context, read_word(physical + 0x20));
    if (!primary)
        return 0;
    write_word(primary->bytes + 0x28, 0x01800180);

    u32 accessory_key = services->allocate_model(services->context);
    if (accessory_key) {
        services->configure(services->context, accessory_key, MINE_ROTOR_ACCESSORY_MODEL);
        MineRotorModelFields *accessory = services->model(services->context, accessory_key);
        if (!accessory)
            return 0;
        write_half(accessory->bytes + 8, read_half(physical + 6));
        write_half(accessory->bytes + 10, (u16)(read_half(physical + 10) + 192));
        write_half(accessory->bytes + 12, (u16)(read_half(physical + 14) - 176));
    }
    if (!mine_rotor_spawn_children(actor, &services->children))
        return 0;
    write_half(physical + 2, (u16)(read_half(physical + 2) + 1));
    write_half(physical + 0x34, 0);
    return 1;
}
#endif
