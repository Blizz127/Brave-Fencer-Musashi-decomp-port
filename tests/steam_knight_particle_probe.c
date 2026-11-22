#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "steam_knight_particle.h"

SteamKnightParticleTransform D_800AE620;
SteamKnightParticleLaunch D_80196874[8];
SteamKnightParticlePosition D_801E2EF0;
u16 D_800B99DC, D_800B99D8;

enum Layout { SEPARATE, MODEL_HANDLE_OVERLAP, OFFSET_OVERLAP };
typedef union {
    SteamKnightPart part;
    struct { u8 pad[0x1c]; SteamKnightPart child; } view;
} OwnerStorage;
typedef union {
    SteamKnightPart child;
    struct { u8 pad[4]; SteamKnightParticlePosition position; } input;
} OffsetStorage;
_Static_assert(offsetof(OwnerStorage, view.child) == 0x1c, "overlap child start");
_Static_assert(offsetof(OffsetStorage, input.position) == 4, "overlap offset start");

static OwnerStorage owner;
static OffsetStorage overlapping_offset;
static SteamKnightPart children[8];
static SteamKnightPart *allocated;
static SteamKnightParticleTransform *working;
static u8 models[2][0x54];
static u32 handles[2];
static unsigned attempts, successes, limit, success_mask;
static int rotated, initialized;
static enum Layout layout;
static u16 observed[8][3];

static void write_u32(void *object, unsigned offset, u32 value)
{
    memcpy((u8 *)object + offset, &value, sizeof value);
}

static u16 child_coord(const void *child, unsigned axis)
{
    u16 value;
    memcpy(&value, (const u8 *)child + 6 + 4 * axis, sizeof value);
    return value;
}

static void set_offset(SteamKnightParticlePosition *p, u16 x, u16 y, u16 z)
{
    const u16 values[] = {x, y, z};
    for (unsigned i = 0; i < 3; ++i) {
        p->bytes[2*i] = (u8)values[i];
        p->bytes[2*i + 1] = (u8)(values[i] >> 8);
    }
}

/* These handles identify two test-owned model buffers independently of
 * their host addresses. Unknown handles are a fixture failure. */
const u8 *bfm_steam_knight_particle_model(u32 handle)
{
    assert(allocated != NULL && !initialized);
    if (handle == handles[0]) return models[0];
    assert(handle == handles[1]);
    return models[1];
}

void func_8004A0DC(u16 angle, SteamKnightParticleTransform *transform)
{
    assert(!rotated && attempts == 0 && angle == 11);
    rotated = 1;
    assert(transform != &D_800AE620);
    assert(memcmp(transform, &D_800AE620, sizeof *transform) == 0);
    working = transform;
    working->words[3] = 120;
    D_800AE620.words[3] = 91;
    D_800B99DC = 65530;
    D_800B99D8 = 12;
    set_offset(&D_801E2EF0, 3, 14, 22);
}

SteamKnightPart *func_8012C658(int kind, u32 selector, int mode)
{
    unsigned row = attempts++;
    assert(working != NULL && allocated == NULL && !initialized);
    assert(row < limit && row < 8 && kind == 84 && mode == 0);
    assert(D_800B99DC == 6);
    assert(selector == (row ? 70000u + row : 101u));
    if (row + 1 < 8) D_80196874[row + 1].selector = 70001u + row;
    if (!(success_mask & (1u << row))) return NULL;

    /* The first coordinate in this view shares two bytes with parent+0x20. */
    allocated = &children[row];
    if (layout == MODEL_HANDLE_OVERLAP) allocated = &owner.view.child;
    if (layout == OFFSET_OVERLAP) allocated = &overlapping_offset.child;
    return allocated;
}

void func_8012B23C(s32 *child)
{
    assert(allocated != NULL && (void *)child == (void *)allocated && !initialized);
    for (unsigned i = 0; i < 3; ++i)
        observed[attempts - 1][i] = child_coord(child, i);
    initialized = 1;
}

void func_800484EC(SteamKnightParticleTransform *transform,
                   SteamKnightParticleLaunch *launch, void *velocity)
{
    unsigned row = attempts - 1;
    assert(initialized && transform == working);
    assert(launch == &D_80196874[row]);
    assert(launch->vector[0] == -(s32)(19 + row));
    assert(velocity == (u8 *)allocated + 0x10);
    assert(working->words[3] == 120 + successes);
    write_u32(velocity, 0, 9000 + row);
    ++working->words[3];
    ++successes;
    if (layout == SEPARATE) write_u32(&owner.part, 0x20, handles[1]);
    allocated = NULL;
    initialized = 0;
}

static void prepare(unsigned expected, unsigned mask, enum Layout shape)
{
    const s32 first[] = {65532, -20, 175};
    const s32 second[] = {-32771, INT32_MAX, INT32_MIN + 1};
    const u16 replacement = 5;
    layout = shape;
    limit = expected;
    success_mask = mask;
    working = NULL;
    allocated = NULL;
    attempts = successes = 0;
    rotated = initialized = 0;
    handles[0] = 0x12a73c95;
    handles[1] = handles[0];
    memcpy((u8 *)&handles[1] + 2, &replacement, sizeof replacement);
    assert(handles[0] != handles[1]);
    assert((uintptr_t)models[0] != handles[0] && (uintptr_t)models[1] != handles[1]);
    for (unsigned i = 0; i < 3; ++i) {
        write_u32(models[0], 0x48 + 4*i, (u32)first[i]);
        write_u32(models[1], 0x48 + 4*i, (u32)second[i]);
    }
    memset(children, 0xa7, sizeof children);
    memset(&owner.part, 0x51, sizeof owner.part);
    memset(observed, 0, sizeof observed);
    write_u32(&owner.part, 0x20, handles[0]);
    for (unsigned i = 0; i < 8; ++i) {
        D_800AE620.words[i] = 25 + 3*i;
        D_80196874[i].vector[0] = -(s32)(19 + i);
        D_80196874[i].selector = 101 + i;
    }
    D_800B99DC = 11;
    D_800B99D8 = 1;
    set_offset(&D_801E2EF0, 9, 28, 65534);
}

static void check_count(unsigned expected_successes)
{
    assert(rotated && D_800B99DC == 6 && D_800AE620.words[3] == 91);
    assert(attempts == limit && successes == expected_successes);
    assert(allocated == NULL && !initialized);
    working = NULL;
}

static void check_pool(int count, unsigned expected, unsigned mask, int wrapper)
{
    /* Wrapper copies the offset before rotation. Direct callers using the
     * global offset observe the replacement written by the rotation service. */
    const u16 copied[][3] = {{5, 8, 173}, {32774, 27, 65535}};
    const u16 live[][3] = {{65535, 65530, 197}, {32768, 13, 23}};
    const u16 (*points)[3] = wrapper ? copied : live;
    unsigned completed = 0;
    prepare(expected, mask, SEPARATE);
    if (wrapper) func_80187664(mask ? &owner.part : NULL);
    else func_8018B16C(mask ? &owner.part : NULL, count, mask ? &D_801E2EF0 : NULL);

    for (unsigned i = 0; i < 8; ++i) {
        SteamKnightPart wanted;
        memset(&wanted, 0xa7, sizeof wanted);
        if (i < expected && (mask & (1u << i))) {
            const u16 *coords = points[completed != 0];
            assert(memcmp(observed[i], coords, sizeof observed[i]) == 0);
            for (unsigned j = 0; j < 3; ++j)
                memcpy((u8 *)&wanted + 6 + 4*j, &coords[j], sizeof coords[j]);
            write_u32(&wanted, 0x10, 9000 + i);
            ++completed;
        }
        assert(memcmp(&children[i], &wanted, sizeof wanted) == 0);
    }
    check_count(completed);
}

static void check_aliases(void)
{
    SteamKnightParticlePosition position;
    prepare(1, 1, MODEL_HANDLE_OVERLAP);
    set_offset(&position, 9, 28, 65534);
    func_8018B16C(&owner.part, 1, &position);
    assert(observed[0][0] == 5 && observed[0][1] == 27 && observed[0][2] == 65535);
    check_count(1);

    prepare(1, 1, OFFSET_OVERLAP);
    set_offset(&overlapping_offset.input.position, 9, 28, 65534);
    func_8018B16C(&owner.part, 1, &overlapping_offset.input.position);
    u16 stored_x = 5;
    u8 bytes[2];
    memcpy(bytes, &stored_x, sizeof bytes);
    u16 next_offset = (u16)(bytes[0] | ((u16)bytes[1] << 8));
    assert(observed[0][0] == stored_x);
    assert(observed[0][1] == (u16)(next_offset - 20));
    assert(observed[0][2] == 173);
    check_count(1);
}

int main(void)
{
    static const struct { int count; unsigned attempts; } counts[] = {
        {INT_MIN, 0}, {-3, 0}, {0, 0}, {1, 1}, {8, 8}, {0x08000000, 1},
        {0x10000000, 1}, {0x10000003, 3}, {INT_MAX, 1}
    };
    for (unsigned mask = 0; mask < 64; ++mask) {
        check_pool(6, 6, mask, 1);
        check_pool(6, 6, mask, 0);
    }
    for (unsigned i = 0; i < sizeof counts / sizeof counts[0]; ++i) {
        check_pool(counts[i].count, counts[i].attempts, 0, 0);
        check_pool(counts[i].count, counts[i].attempts, 255, 0);
    }
    check_aliases();
    return 0;
}
