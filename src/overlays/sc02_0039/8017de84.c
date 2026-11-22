/* Initialize a mine collision actor or its linked child model. */
#include "mine_collision_actor_setup.h"
#if BFM_MINE_DIRECT_GUEST_ADDRESS
#include "model_surface_probe.h"

typedef struct {
    u16 type;
    u16 state;
    u8 before_x[2];
    s16 x;
    u8 before_y[2];
    s16 y;
    u8 before_z[2];
    s16 z;
    u8 pad10[16];
    u32 model;
    u8 pad24[52];
    u32 geometry;
    u16 flags;
    u8 pad5e[6];
    u32 parent;
    u8 pad68[8];
    u16 options;
    u8 pad72[3];
    u8 mode75;
    u8 pad76[136];
    s16 variant;
    s16 parameter;
} Actor;
typedef struct {
    u32 reserved;
    u32 flags;
} Model;

/* The currently compiled game service units declare their own untagged record views.
 * Keep their member widths and names compatible at this historical ABI boundary. */
typedef struct {
    s16 unk0, unk2;
    u8 pad4[0x1c];
    s32 unk20, unk24;
} ListModel;
typedef struct {
    s16 unk0, unk2;
    u8 pad4[0x20];
    s32 unk24;
} ChildModel;
typedef struct {
    u8 pad[0x30];
    s32 unk30;
    u8 pad34[0x44];
    void *unk78;
} LinkModel;
typedef struct {
    u16 id;
    u8 p2[4];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0xfc];
} SpawnActor;
typedef struct {
    u8 pad0[2];
    u16 cnt;
    u8 pad4[0x30];
    u16 clr;
} StateActor;

extern s32 D_8019CDBC[], D_8019D81C[];
extern SceneSurfaceBox D_8019B57C;
extern void *func_8012C1B8(void);
extern s32 func_8012CAE4(u8 *actor);
extern void func_8001C810(ListModel *model, s32 *list);
extern s32 func_8012C658(s16 kind, s16 variant, SpawnActor *parent);
extern void func_8001C2C4(ChildModel *model);
extern void func_8001C1E4(LinkModel *model, u8 *parent_model);
extern void func_8012AD50(StateActor *actor);

void func_8017DE84(Obj8014A2E4 *entity)
{
    Actor *actor = (Actor *)entity;
    actor->model = (u32)func_8012C1B8();
    if (!actor->model) {
        func_8012CAE4((u8 *)actor);
        return;
    }
    actor->variant = actor->options & 3;
    switch (actor->variant) {
    case 0:
        func_8001C810((ListModel *)actor->model, D_8019CDBC);
        ((Model *)actor->model)->flags |= 0x8040;
        func_8012C658(0x199, 2, (SpawnActor *)actor);
        break;
    case 1:
        func_8001C810((ListModel *)actor->model, D_8019D81C);
        ((Model *)actor->model)->flags |= 0x8040;
        func_8012C658(0x199, 3, (SpawnActor *)actor);
        break;
    case 2:
    case 3:
        func_8001C2C4((ChildModel *)actor->model);
        func_8001C1E4((LinkModel *)actor->model, (u8 *)((Actor *)actor->parent)->model);
        actor->y = 144;
        actor->geometry = (u32)&D_8019B57C | 0x40000000;
        actor->flags = 0x4400;
        actor->x = 0;
        actor->z = 0;
        actor->mode75 = 4;
        break;
    }
    actor->parameter = actor->options & 0xff0;
    ((Model *)actor->model)->flags |= 0x8000;
    func_8012AD50((StateActor *)actor);
}
#else

static uint32_t load32(const uint8_t *field)
{
    return field[0] | (uint32_t)field[1] << 8 | (uint32_t)field[2] << 16 |
           (uint32_t)field[3] << 24;
}

static void store32(uint8_t *field, uint32_t value)
{
    field[0] = (uint8_t)value;
    field[1] = (uint8_t)(value >> 8);
    field[2] = (uint8_t)(value >> 16);
    field[3] = (uint8_t)(value >> 24);
}

static uint16_t load16(const uint8_t *field)
{
    return (uint16_t)(field[0] | (uint16_t)field[1] << 8);
}

static void store16(uint8_t *field, uint16_t value)
{
    field[0] = (uint8_t)value;
    field[1] = (uint8_t)(value >> 8);
}

static int model_flags(const MineSetupServices *services, uint32_t key, uint32_t bits)
{
    if (!services->model_flags)
        return 0;
    uint8_t *field = services->model_flags(services->context, key);
    if (!field)
        return 0;
    store32(field, load32(field) | bits);
    return 1;
}

int mine_setup_actor(Obj8014A2E4 *actor, const MineSetupServices *services)
{
    if (!actor || !services || !services->allocate)
        return 0;
    uint8_t *data = (uint8_t *)actor;
    void *context = services->context;
    uint32_t model = services->allocate(context);
    store32(data + 0x20, model);
    if (!model) {
        if (!services->remove_actor)
            return 0;
        services->remove_actor(context, actor);
        return 1;
    }

    uint16_t variant = load16(data + 0x70) & 3;
    store16(data + 0xfe, variant);
    if (variant < 2) {
        if (!services->configure)
            return 0;
        services->configure(context, model, variant ? MINE_SETUP_LIST_B : MINE_SETUP_LIST_A);
        if (!model_flags(services, load32(data + 0x20), 0x8040) || !services->spawn)
            return 0;
        services->spawn(context, 0x199, (int16_t)(2 + variant), actor);
    } else {
        if ((services->box_address & UINT32_C(0xf0000003)) != UINT32_C(0x80000000) ||
            !services->prepare_child)
            return 0;
        services->prepare_child(context, model);
        if (!services->actor)
            return 0;
        const Obj8014A2E4 *parent = services->actor(context, load32(data + 0x64));
        if (!parent || !services->link_child)
            return 0;
        services->link_child(context, load32(data + 0x20),
                             load32((const uint8_t *)parent + 0x20));
        store16(data + 0x0a, 144);
        store32(data + 0x58, services->box_address | UINT32_C(0x40000000));
        store16(data + 0x5c, 0x4400);
        store16(data + 0x06, 0);
        store16(data + 0x0e, 0);
        data[0x75] = 4;
    }

    /* Earlier services may have replaced the model or changed the options. */
    store16(data + 0x100, load16(data + 0x70) & 0xff0);
    if (!model_flags(services, load32(data + 0x20), 0x8000))
        return 0;
    /* The state advance increments the state and clears its timer. */
    store16(data + 2, (uint16_t)(load16(data + 2) + 1));
    store16(data + 0x34, 0);
    return 1;
}
#endif
