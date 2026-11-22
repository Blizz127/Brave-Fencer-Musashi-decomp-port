#include "mine_child_query.h"
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(SceneProbeVector) == 8, "vector spans four halfwords");
_Static_assert(offsetof(MineChildQueryShape, record) == 0, "public prefix");
_Static_assert(offsetof(MineChildQueryShape, samples) == 8, "sample origin");
_Static_assert(sizeof(MineChildModelActor) == 0x10c, "full actor slot");

enum QueryCase { SIMPLE_GRID, CHANGING_GRID, LONG_ROW };
static enum QueryCase current_case;
static MineChildQueryShape *shape;
static MineChildModelActor actor;
static SceneProbeVector *seen_start, *seen_end;
static unsigned int calls, syncs, hit, capacity;
static s32 hit_value;

#ifdef BFM_TEST_QUERY_CALLER
static MineChildQueryShape *alternate;
MineChildModelRecord *D_8019B618[3];
u16 D_80126B96, D_80126B98;
#endif

static MineChildQueryShape *make_shape(size_t allocated_samples, u16 initial_count)
{
    MineChildQueryShape *result;
    size_t base = sizeof(MineChildQueryShape);
    if (initial_count > allocated_samples ||
        allocated_samples > (SIZE_MAX - base) / sizeof(u16))
        return NULL;
    result = calloc(1, base + allocated_samples * sizeof(u16));
    assert(result);
    result->record.count = initial_count;
    return result;
}

static s16 signed_sample(u16 bits)
{
    return (s16)(bits <= 0x7fffu ? (int)bits : (int)bits - 0x10000);
}

static u16 long_sample(unsigned index)
{
    return (u16)(index * 97u + 0x8113u);
}

static void reset_query(enum QueryCase which, unsigned allocated, u16 count, u16 width)
{
    shape = make_shape(allocated, count);
    assert(shape && (unsigned)count <= allocated);
    capacity = allocated;
    shape->record.extent = width;
    shape->record.feedback_a = 0x4567;
    shape->record.feedback_b = 0xef23;
    current_case = which;
    calls = syncs = 0;
    seen_start = seen_end = NULL;
    hit = UINT_MAX;
    hit_value = 1;
    memset(&actor, 0x71, sizeof actor);
    actor.model_index = 0;
}

static void check_vectors(SceneProbeVector *start, SceneProbeVector *end,
                          s16 ax, s16 bx, s16 ay, s16 by, s16 z)
{
    assert(start->x == ax && end->x == bx);
    assert(start->y == ay && end->y == by);
    assert(start->z == z && end->z == z);
}

static void change_query(SceneProbeVector *start, SceneProbeVector *end, unsigned at)
{
    assert(capacity == 4);
    if (at) {
        assert(start->pad == -128 && end->pad == 257);
        assert(actor.before_model_index[91] == 0x4b);
    }
    switch (at) {
    case 0:
        check_vectors(start, end, -4, 4, -101, -101, -101);
        start->x = 510; end->x = -520;
        start->y = 610; end->y = -620;
        start->pad = -128; end->pad = 257;
        shape->record.extent = 1;
        shape->record.count = 3;
        shape->samples[0] = 301; shape->samples[1] = -1001;
        actor.before_model_index[91] = 0x4b;
        break;
    case 1:
        check_vectors(start, end, 510, -520, 610, -620, -1001);
        start->y = 711; end->y = -722;
        shape->samples[2] = -4073;
        shape->samples[1] = 2222;
        break;
    case 2:
        check_vectors(start, end, 510, -520, 711, -722, -4073);
        start->x = 315; end->x = -317;
        start->y = 123; end->y = 456;
        shape->record.count = 2;
        shape->samples[1] = 3210;
        break;
    case 3:
        check_vectors(start, end, 315, -317, 3210, 3210, 301);
        start->y = 712; end->y = -723;
        shape->samples[1] = -2999;
        break;
    case 4:
        check_vectors(start, end, 315, -317, 712, -723, -2999);
        shape->record.count = 0;
        shape->record.feedback_a = 0xe183;
        shape->record.feedback_b = 0x8b1f;
        break;
    default:
        abort();
    }
    start->z = -1661;
    end->z = 2981;
}

s32 bfm_mine_child_query_segment(MineChildModelActor *owner,
                                 SceneProbeVector *start, SceneProbeVector *end)
{
    unsigned at = calls++;
    assert(owner == &actor && start && end && start != end);
    if (at == 0) {
        seen_start = start;
        seen_end = end;
    } else {
        assert(start == seen_start && end == seen_end);
    }
    if (current_case == CHANGING_GRID) {
        change_query(start, end, at);
    } else {
        unsigned n = shape->record.count;
        assert(n && n <= capacity);
        s16 extent = (s16)(shape->record.extent / 2u);
        unsigned row = at / n, column = at % n;
        assert(row < n);
        s16 y = shape->samples[row];
        s16 z = shape->samples[column];
        check_vectors(start, end, (s16)-extent, extent, y, y, z);
        if (current_case == LONG_ROW) {
            assert(row == 0);
            assert(z == signed_sample(long_sample(column)));
        }
    }
    return at == hit ? hit_value : 0;
}

static void fixed_grid(u16 n, u16 extent, unsigned positive, s32 raw)
{
    static const u16 coords[] = {0x8000, 0x7fff, 0xfffd, 0x4210};
    reset_query(SIMPLE_GRID, n, n, extent);
    for (unsigned i = 0; i < n; ++i) shape->samples[i] = signed_sample(coords[i]);
    hit = positive;
    hit_value = raw;
    unsigned limit = (unsigned)n * n;
    int success = raw && positive < limit;
    assert(func_8017EF5C(&actor, &shape->record) == success);
    assert(calls == (success ? positive + 1 : limit));
    free(shape);
}

static void begin_changes(void)
{
    reset_query(CHANGING_GRID, 4, 2, 9);
    shape->samples[0] = -101;
    shape->samples[1] = 92;
    shape->samples[2] = -8192;
    shape->samples[3] = 32222;
    hit = 4;
}

static void changing_grid(s32 raw)
{
    begin_changes();
    hit_value = raw;
    assert(func_8017EF5C(&actor, &shape->record) == (raw != 0));
    assert(calls == 5 && shape->record.count == 0);
    assert(shape->record.feedback_a == 0xe183 && shape->record.feedback_b == 0x8b1f);
    free(shape);
}

static void long_row(u16 n, unsigned positive)
{
    reset_query(LONG_ROW, n, n, 65535);
    for (unsigned i = 0; i < n; ++i) shape->samples[i] = signed_sample(long_sample(i));
    hit = positive;
    hit_value = INT32_MIN;
    assert(func_8017EF5C(&actor, &shape->record) == 1);
    assert(calls == positive + 1);
    free(shape);
}

#ifdef BFM_TEST_QUERY_CALLER
void func_8012B2CC(char *owner)
{
    assert((void *)owner == (void *)&actor && syncs++ == 0 && calls == 0);
    assert(D_8019B618[0] == &shape->record && D_8019B618[1] == &alternate->record);
    actor.model_index = 1;
    D_8019B618[0] = &alternate->record;
    D_80126B96 = 0x81a1;
    D_80126B98 = 0x3c0b;
}

static void retained_record(s32 raw)
{
    begin_changes();
    hit_value = raw;
    alternate = make_shape(1, 1);
    assert(alternate);
    alternate->record.feedback_a = 0x4321;
    alternate->record.feedback_b = 0x8765;
    D_8019B618[0] = &shape->record;
    D_8019B618[1] = &alternate->record;
    D_8019B618[2] = NULL;
    func_8017EED8(&actor);
    assert(syncs == 1 && calls == 5 && actor.model_index == 1);
    assert(D_80126B96 == (raw ? 0xe183 : 0x81a1));
    assert(D_80126B98 == (raw ? 0x8b1f : 0x3c0b));
    actor.model_index = 2;
    func_8017EED8(&actor);
    assert(syncs == 1 && calls == 5);
    free(alternate); free(shape);
}
#endif

int main(void)
{
    assert(make_shape(1, 2) == NULL && make_shape(SIZE_MAX, 0) == NULL);
    reset_query(SIMPLE_GRID, 1, 1, 0xffff);
    assert(func_8017EF5C(NULL, &shape->record) == 0);
    assert(func_8017EF5C(&actor, NULL) == 0);
    assert(func_8017EF5C(NULL, NULL) == 0 && calls == 0);
    free(shape);
    fixed_grid(0, 65535, 0, 1);
    fixed_grid(1, 0, 0, 0);
    fixed_grid(1, 1, 0, -1);
    for (unsigned i = 0; i <= 9; ++i)
        fixed_grid(3, (i & 1) ? 0x8001 : 0xffff, i, i & 1 ? INT32_MIN : 32768);
    fixed_grid(4, 373, 8, 0);
    changing_grid(0);
    changing_grid(INT32_MIN);
    long_row(32768, 32767);
    long_row(65535, 65534);
#ifdef BFM_TEST_QUERY_CALLER
    retained_record(INT32_MIN);
    retained_record(0);
#endif
    return 0;
}
