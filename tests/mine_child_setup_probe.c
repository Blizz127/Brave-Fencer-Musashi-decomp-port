#include "mine_child_setup.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Give every callback a complete synthetic actor or model slot, including tail bytes. */
typedef struct {
    MineChildSetup state;
} Actor;
typedef struct {
    MineChildSetupModel state;
    u8 tail[132 - sizeof(MineChildSetupModel)];
} Model;
typedef struct {
    Actor child, parents[2];
    Model models[3];
} World;

_Static_assert(sizeof(Actor) == 268 && sizeof(Model) == 132, "complete slots");
_Static_assert(offsetof(MineChildSetup, z) == 0x0e && offsetof(MineChildSetup, position) == 0x10, "motion");
_Static_assert(offsetof(MineChildSetup, model) == 0x20 && offsetof(MineChildSetup, motion) == 0x48, "child model and motion");
_Static_assert(offsetof(MineChildSetup, shape) == 0x58 && offsetof(MineChildSetup, contact_flags) == 0x5c, "contact");
_Static_assert(offsetof(MineChildSetup, parent) == 0x64 && offsetof(MineChildSetup, subtype) == 0x70, "parent and subtype");
_Static_assert(offsetof(MineChildSetup, state_byte) == 0x75 && offsetof(MineChildSetup, step) == 0x100 && offsetof(MineChildSetup, variant) == 0x102, "stage fields");
_Static_assert(offsetof(MineChildSetupModel, yaw) == 0x12 && offsetof(MineChildSetupModel, scale_x) == 0x18, "model orientation and scale");
_Static_assert(offsetof(MineChildSetupModel, flags) == 0x2c && offsetof(MineChildSetupModel, transform) == 0x34, "model transform");

MineChildSetupRow D_8019B5B4[3];
MineChildSetupRate D_8019B5E6[3];
s32 D_8019E298[4];
u32 bfm_mine_child_setup_shape_address;
static const u32 model_ids[] = {0xb4142100u, 0x3818feb4u, 0xe07c1140u};
static const u32 parent_ids[] = {0x591247ac, 0xdb8646d4u};
static World world, expected;

enum Service { ALLOCATE, REMOVE, BIND, SCALE_RANDOM, YAW_RANDOM, SYNC, TRANSFORM, HEIGHT_RANDOM, ADVANCE };
static enum Service plan[9];
static unsigned at, length;
static s32 results[3];
static unsigned variant, changes;
static int allocated;
enum Stop { FINISH, ZERO_DIVISOR, BAD_EARLY_MODEL, BAD_SCALE_MODEL, BAD_SCALE_INDEX, BAD_BOUND_INDEX,
            BAD_PARENT, BAD_PARENT_MODEL, BAD_YAW_MODEL, BAD_TRANSFORM_MODEL, BAD_TRANSFORM_INDEX };
static enum Stop stop;
static const u32 missing = 0x789cfb12;

static void compare(void)
{
    assert(memcmp(&world, &expected, sizeof world) == 0);
}

static void service(enum Service which)
{
    assert(at < length && plan[at] == which);
    ++at;
    compare();
}

static Model *model_at(World *w, u32 id)
{
    for (unsigned i = 0; i < 3; ++i)
        if (id == model_ids[i]) return &w->models[i];
    return NULL;
}

static Actor *parent_at(World *w, u32 id)
{
    for (unsigned i = 0; i < 2; ++i)
        if (id == parent_ids[i]) return &w->parents[i];
    return NULL;
}

MineChildSetupModel *bfm_mine_child_setup_model(u32 id)
{
    Model *m = model_at(&world, id);
    return m ? &m->state : NULL;
}

MineChildSetup *bfm_mine_child_setup_parent(u32 id)
{
    Actor *p = parent_at(&world, id);
    return p ? &p->state : NULL;
}

static s16 halfword(u32 v)
{
    unsigned low = v & 65535u;
    return (s16)(low >= 32768 ? (int)low - 65536 : (int)low);
}

static void accept_external_writes(void)
{
    memcpy(&expected, &world, sizeof world);
}

u32 bfm_mine_emitter_acquire_model(void)
{
    service(ALLOCATE);
    world.child.state.tail[1] ^= 0x59;
    world.child.state.model = model_ids[2];
    accept_external_writes();
    return allocated ? model_ids[0] : 0;
}

s32 func_8012CAE4(u8 *actor)
{
    assert(actor == (u8 *)&world.child.state);
    expected.child.state.model = 0;
    service(REMOVE);
    world.child.state.state_byte ^= 0x7c;
    accept_external_writes();
    return INT32_MIN;
}

void bfm_mine_child_setup_bind(u32 id, s32 *descriptor)
{
    assert(id == model_ids[0] && descriptor == D_8019E298);
    expected.child.state.model = id;
    service(BIND);
    world.child.state.subtype = (u16)(0xe930u | variant);
    if (changes) world.child.state.model = model_ids[1];
    if (stop == BAD_EARLY_MODEL) world.child.state.model = missing;
    if (stop == ZERO_DIVISOR) D_8019B5E6[variant].divisor = 0;
    accept_external_writes();
}

static void expect_scale(void)
{
    MineChildSetup *child = &expected.child.state;
    if (variant) {
        MineChildSetupModel *model = &model_at(&expected, child->model)->state;
        u32 scale = D_8019B5B4[child->variant].scale + ((u32)results[0] & 255);
        model->scale_x = model->scale_y = model->scale_z = halfword(scale);
        model->flags |= 0x10u;
    }
    MineChildSetupModel *model = &model_at(&expected, child->model)->state;
    child->motion = 98304;
    child->step = model->scale_x >= 4096 ? 64 : (s16)((5120 - model->scale_x) / 16);
}

s32 func_8005C49C(void)
{
    assert(at < length);
    switch (plan[at]) {
    case SCALE_RANDOM:
        expected.child.state.variant = (s16)variant;
        service(SCALE_RANDOM);
        if (changes) {
            world.child.state.model = model_ids[2];
            world.child.state.variant = (s16)(3u - variant);
        }
        if (stop == BAD_SCALE_MODEL) world.child.state.model = missing;
        if (stop == BAD_SCALE_INDEX) world.child.state.variant = -7;
        accept_external_writes();
        return results[0];
    case YAW_RANDOM:
        if (!variant) expected.child.state.variant = 0;
        expect_scale();
        service(YAW_RANDOM);
        if (changes) {
            world.child.state.model = model_ids[0];
            world.child.state.parent = parent_ids[1];
            world.parents[1].state.model = model_ids[0];
            world.models[0].state.yaw = 17;
            /* The choice of yaw formula was made before the call. */
            world.child.state.variant = (s16)((world.child.state.variant + 1) % 3);
        }
        if (stop == BAD_PARENT) world.child.state.parent = missing;
        if (stop == BAD_PARENT_MODEL) parent_at(&world, world.child.state.parent)->state.model = missing;
        if (stop == BAD_YAW_MODEL) world.child.state.model = missing;
        accept_external_writes();
        return results[1];
    case HEIGHT_RANDOM:
        service(HEIGHT_RANDOM);
        if (changes) world.child.state.z = 9;
        world.child.state.contact_flags = 0x713;
        world.child.state.state_byte = 0x55;
        accept_external_writes();
        return results[2];
    default:
        assert(0 && "unexpected random call");
        return 0;
    }
}

void func_8012B2CC(char *actor)
{
    assert((void *)actor == (void *)&world.child.state);
    MineChildSetup *child = &expected.child.state;
    MineChildSetup *parent = &parent_at(&expected, child->parent)->state;
    unsigned base = model_at(&expected, parent->model)->state.yaw;
    unsigned chosen = changes && variant ? 3u - variant : variant;
    int delta = chosen == 2 ? ((results[1] & 1) ? 224 : -224) : (int)((u32)results[1] & 255u) - 128;
    model_at(&expected, child->model)->state.yaw = (u16)((int)base + delta);
    service(SYNC);
    if (changes) {
        world.child.state.model = model_ids[1];
        world.child.state.variant = 2;
        D_8019B5B4[2].offset[1] = INT32_MIN + 7;
    }
    if (stop == BAD_TRANSFORM_MODEL) world.child.state.model = missing;
    if (stop == BAD_TRANSFORM_INDEX) world.child.state.variant = 3;
    accept_external_writes();
}

void bfm_mine_child_setup_transform(void *transform, void *offset, void *position)
{
    assert(transform == world.models[changes ? 1 : 0].state.transform);
    assert(offset == &D_8019B5B4[world.child.state.variant]);
    assert(position == world.child.state.position);
    service(TRANSFORM);
    world.child.state.position[1] = D_8019B5B4[world.child.state.variant].offset[1];
    world.child.state.z = 131;
    world.models[1].tail[3] ^= 0xe1;
    accept_external_writes();
}

void func_8012AD50(MineEmitterStateActor *actor)
{
    assert((void *)actor == (void *)&world.child.state);
    MineChildSetup *child = &expected.child.state;
    child->shape = bfm_mine_child_setup_shape_address | 0x60000000u;
    child->contact_flags = 0xc000;
    child->state_byte = 0;
    child->z = (u16)(child->z - ((u32)results[2] & 31u));
    service(ADVANCE);
    world.child.state.stage.stage = 1;
    world.child.state.tail[2] ^= 0x31;
    accept_external_writes();
}

#ifdef BFM_TEST_CHILD_SELECTOR
static unsigned stage_one_count;
static void observe_stage_one(MineActorStage *actor)
{
    assert(actor == &world.child.state.stage && actor->stage == 1);
    ++stage_one_count;
}
MineStageAction D_8019B634[2] = {func_8017EA20, observe_stage_one};
#endif

static void run(unsigned index, unsigned vary, int has_model, unsigned seed, u32 scale, s32 yaw)
{
    u8 *p = (u8 *)&world;
    for (size_t i = 0; i < sizeof world; ++i) p[i] = (u8)(173 * i + 31 * seed);
    variant = index;
    changes = vary;
    allocated = has_model;
    at = length = 0;
    plan[length++] = ALLOCATE;
    if (!allocated) plan[length++] = REMOVE;
    else {
        plan[length++] = BIND;
        if (index) plan[length++] = SCALE_RANDOM;
        plan[length++] = YAW_RANDOM;
        plan[length++] = SYNC;
        plan[length++] = TRANSFORM;
        plan[length++] = HEIGHT_RANDOM;
        plan[length++] = ADVANCE;
    }
    if (stop != FINISH) {
        enum Service last = (stop == ZERO_DIVISOR || stop == BAD_EARLY_MODEL) ? BIND :
                            (stop == BAD_SCALE_MODEL || stop == BAD_SCALE_INDEX || stop == BAD_BOUND_INDEX) ? SCALE_RANDOM :
                            (stop == BAD_TRANSFORM_MODEL || stop == BAD_TRANSFORM_INDEX) ? SYNC : YAW_RANDOM;
        while (length && plan[length - 1] != last) --length;
        assert(length);
    }
    world.child.state.stage.stage = 0;
    world.child.state.parent = parent_ids[0];
    world.child.state.subtype = 0xfffa;
    world.parents[0].state.model = model_ids[2];
    world.models[2].state.yaw = 65532;
    for (unsigned i = 0; i < 3; ++i) {
        world.models[i].state.scale_x = halfword(scale);
        world.models[i].state.flags = (u16)(0x6a81u + i);
        D_8019B5B4[i].scale = scale + 0x20000u * i;
        D_8019B5B4[i].offset[1] = (s32)i - 17;
        D_8019B5E6[i].divisor = (s16)(i == 0 ? -3 : i == 1 ? 127 : -1024);
    }
    bfm_mine_child_setup_shape_address = 0x9371ad4u ^ seed;
    results[0] = seed & 1u ? INT32_MIN : -1;
    results[1] = yaw;
    results[2] = seed & 1u ? -1 : INT32_MIN;
    accept_external_writes();
#ifdef BFM_TEST_CHILD_SELECTOR
    func_8017EE9C(&world.child.state.stage);
    if (allocated && stop == FINISH) {
        unsigned previous = stage_one_count;
        func_8017EE9C(&world.child.state.stage);
        assert(stage_one_count == previous + 1);
    }
#else
    func_8017EA20(&world.child.state.stage);
#endif
    assert(at == length);
    if (stop == ZERO_DIVISOR || stop == BAD_EARLY_MODEL) {
        expected.child.state.variant = 0;
        expected.child.state.motion = 98304;
        if (stop == BAD_EARLY_MODEL)
            expected.child.state.step = (s16)(4096 / D_8019B5E6[0].divisor);
    }
    compare();
}

int main(void)
{
    static const u32 scales[] = {0, 4095, 4096, 0x7fffu, 0x8000u, 0xffffffffu};
    static const s32 yaws[] = {INT32_MIN, -1, 127, 128, 255, INT32_MAX};
    run(0, 1, 0, 81, 64, 17);
    for (unsigned v = 0; v < 3; ++v)
        for (unsigned c = 0; c < 2; ++c)
            for (unsigned i = 0; i < sizeof scales / sizeof scales[0]; ++i)
                run(v, c, 1, 9 + 83 * v + i, scales[i], yaws[i]);
    /* These synthetic guards stop before the next unavailable table or model. */
    for (int which = ZERO_DIVISOR; which <= BAD_TRANSFORM_INDEX; ++which) {
        stop = (enum Stop)which;
        unsigned selected = stop == BAD_BOUND_INDEX ? 8u :
                            stop == BAD_SCALE_MODEL || stop == BAD_SCALE_INDEX ? 1u : 0u;
        run(selected, 0, 1, 83, 4095, -1);
    }
    at = length = 0;
    func_8017EA20(NULL);
    assert(!at);
    return 0;
}
