#include "mine_emitter_projection.h"
#ifdef BFM_TEST_EMITTER_SCAN
#include "mine_emitter_scan.h"
#endif
#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum { SLOTS = 96, MODEL_BYTES = 132, MATRIX_BYTES = 32 };
Obj8014A2E4 D_801202A0[SLOTS];
static Obj8014A2E4 expected_pool[SLOTS];

/* The fixture supplies every transform/projection result using synthetic inputs. */
static const u32 model_id[] = {0xa07133e0u, 0x4f7bff08u};
static unsigned char model_bytes[2][MODEL_BYTES];
static unsigned order[2], actor_count, actor_number, stage, sound_calls;
static unsigned stop_before, scan_case;
static unsigned char expected_matrix[MATRIX_BYTES], expected_vector[8];
static void *matrix_address, *vector_address, *shared_address;
static u32 initial_shared, screen_word, flag_word;
static s32 depth_word;
static jmp_buf held;

static void put16(void *data, size_t offset, u16 value)
{
    unsigned char *b = (unsigned char *)data + offset;
    b[0] = (u8)value;
    b[1] = (u8)(value >> 8);
}
static void put32(void *data, size_t offset, u32 value)
{
    put16(data, offset, (u16)value);
    put16(data, offset + 2, (u16)(value >> 16));
}
static u16 get16(const void *data, size_t offset)
{
    const unsigned char *b = (const unsigned char *)data + offset;
    return (u16)(b[0] + 256u * b[1]);
}
static u32 get32(const void *data, size_t offset)
{
    return get16(data, offset) + ((u32)get16(data, offset + 2) << 16);
}
static int signed_half(u16 bits)
{
    return bits < 0x8000u ? (int)bits : (int)bits - 65536;
}
static Obj8014A2E4 *active(void)
{
    assert(actor_number < actor_count);
    return &D_801202A0[order[actor_number]];
}
static void match_pool(void)
{
    assert(memcmp(D_801202A0, expected_pool, sizeof expected_pool) == 0);
}
static void before_service(unsigned next)
{
    assert(next == stage + 1 && actor_number < actor_count);
    stage = next;
    match_pool();
    if (stop_before == next) longjmp(held, 1);
}
static void retain_actor_edits(void)
{
    memcpy(expected_pool, D_801202A0, sizeof expected_pool);
}
static void paint(void *start, size_t n, unsigned seed)
{
    unsigned char *p = start;
    for (size_t i = 0; i < n; ++i) p[i] = (unsigned char)(seed + 37u * i);
}
static void check_yaw(s32 yaw, void *matrix, void *source_matrix)
{
    before_service(1);
    assert(matrix && matrix != source_matrix);
    Obj8014A2E4 *a = active();
    memcpy(expected_matrix, source_matrix, MATRIX_BYTES);
    for (unsigned i = 0; i < 3; ++i)
        put32(expected_matrix, 20 + i * 4, (u32)signed_half(get16(a, 6 + i * 4)));
    assert(memcmp(matrix, expected_matrix, MATRIX_BYTES) == 0);
    assert(yaw == signed_half(get16(model_bytes[actor_number], 18)));
    matrix_address = matrix;
    /* Changing the global template cannot change the local matrix. */
    paint(source_matrix, MATRIX_BYTES, 21 + actor_number);
    paint(expected_matrix, MATRIX_BYTES, 33 + actor_number);
    memcpy(matrix, expected_matrix, MATRIX_BYTES);
    put16(a, 6, 0x8147);
    put32(a, 32, model_id[1 - actor_number]);
    put16(model_bytes[actor_number], 18, 0x4d10);
    retain_actor_edits();
}
static void check_matrix(unsigned next, void *matrix)
{
    before_service(next);
    assert(matrix == matrix_address && !memcmp(matrix, expected_matrix, MATRIX_BYTES));
    paint(expected_matrix, MATRIX_BYTES, 79 + next + actor_number);
    memcpy(matrix, expected_matrix, MATRIX_BYTES);
}
static void check_vector(void *origin, void *expected_origin, void *out, void *flag)
{
    before_service(4);
    assert(!memcmp(matrix_address, expected_matrix, MATRIX_BYTES));
    assert(origin == expected_origin && out != origin && flag != out);
    assert(memcmp(origin, expected_vector, sizeof expected_vector) == 0);
    vector_address = out;
    shared_address = flag;
    paint(expected_vector, sizeof expected_vector, 17 + actor_number);
    memcpy(out, expected_vector, sizeof expected_vector);
    initial_shared = actor_number ? 0x415379f0u : 0xc5e371e0u;
    put32(flag, 0, initial_shared);
    paint(origin, sizeof expected_vector, 200 + actor_number);
}
static void check_view(void)
{
    before_service(5);
    assert(!memcmp(vector_address, expected_vector, sizeof expected_vector));
    assert(get32(shared_address, 0) == initial_shared);
    expected_vector[3] ^= 0xe2;
    memcpy(vector_address, expected_vector, sizeof expected_vector);
    initial_shared ^= 0x833111a3u;
    put32(shared_address, 0, initial_shared);
    Obj8014A2E4 *a = active();
    put32(a, 0xe0, 0x38999bb4u);
    put16(a, 0x100, 0x2a71);
    retain_actor_edits();
}
static s32 check_project(void *v, void *screen, void *unused_result, void *flag)
{
    before_service(6);
    assert(v == vector_address && flag == shared_address && screen != unused_result);
    assert(screen != flag && unused_result != flag && v != screen && v != unused_result);
    assert(!memcmp(v, expected_vector, sizeof expected_vector));
    assert(get32(flag, 0) == initial_shared);
    if (scan_case) {
        screen_word = actor_number ? 0x81030004u : 0x7408ffecu;
        flag_word = scan_case == 1 && actor_number == 1 ? 0x80000000u : 0;
        depth_word = actor_number ? 336 : -319;
    }
    put32(screen, 0, screen_word);
    put32(unused_result, 0, 0x602702feu);
    put32(flag, 0, flag_word);
    Obj8014A2E4 *a = active();
    ((unsigned char *)a)[0x85] ^= 0x49;
    retain_actor_edits();
    put32(&expected_pool[order[actor_number]], 0xe0, (u32)depth_word);
    put16(&expected_pool[order[actor_number]], 0x100,
          flag_word >= 0x80000000u ? (u16)-999 : (u16)screen_word);
    ++actor_number;
    stage = 0;
    return depth_word;
}

#ifdef BFM_TEST_EMITTER_SCAN
void bfm_mine_emitter_audio(u16 command, u16 argument)
{
    assert(scan_case && actor_number == 2 && !sound_calls++);
    Obj8014A2E4 *expected_owner = &expected_pool[order[1]];
    put16(expected_owner, 0x84, (u16)(get16(expected_owner, 0x84) + 1));
    match_pool();
    if (scan_case == 1) {
        assert(command == 0x6da && argument == 0x3775);
    } else {
        assert(command == 4 && argument == 0x6da);
    }
    put16(&D_801202A0[order[1]], 0x102, 0x8f32);
    retain_actor_edits();
    put16(expected_owner, 0x102, (u16)(scan_case == 1));
}
#endif

MineEmitterProjectionMatrix D_800AE620;
SceneProbeVector D_8019B5AC;
static unsigned lookups;
_Static_assert(sizeof D_800AE620 == MATRIX_BYTES && sizeof D_8019B5AC == 8, "matrix and vector sizes");
_Static_assert(sizeof(Obj8014A2E4) == 268 && offsetof(Obj8014A2E4, probe_object_address) == 32, "actor model word");

int bfm_mine_emitter_model_yaw(u32 id, s16 *yaw)
{
    ++lookups;
    assert(id == active()->probe_object_address);
    if (!id || id != model_id[actor_number]) return 0;
    *yaw = (s16)signed_half(get16(model_bytes[actor_number], 18));
    memcpy(expected_vector, &D_8019B5AC, sizeof expected_vector);
    return 1;
}
void bfm_mine_emitter_rotate(s32 yaw, MineEmitterProjectionMatrix *matrix)
{
    check_yaw(yaw, matrix, &D_800AE620);
}
void bfm_mine_emitter_set_rotation(MineEmitterProjectionMatrix *matrix)
{
    check_matrix(2, matrix);
}
void bfm_mine_emitter_set_translation(MineEmitterProjectionMatrix *matrix)
{
    check_matrix(3, matrix);
}
void bfm_mine_emitter_transform(SceneProbeVector *input, SceneProbeVector *output, s32 *flag)
{
    check_vector(input, &D_8019B5AC, output, flag);
}
s32 func_8012E32C(void)
{
    check_view();
    return INT32_MIN + 21;
}
s32 bfm_mine_emitter_project(SceneProbeVector *v, s32 *screen, s32 *cue, s32 *flag)
{
    return check_project(v, screen, cue, flag);
}

static void setup(unsigned linked, unsigned seed, u16 yaw)
{
    paint(D_801202A0, sizeof D_801202A0, seed + 29);
    for (unsigned i = 0; i < SLOTS; ++i) D_801202A0[i].active = i & 1 ? 425 : 0;
    paint(&D_800AE620, MATRIX_BYTES, seed + 83);
    paint(&D_8019B5AC, sizeof D_8019B5AC, seed + 19);
    paint(model_bytes, sizeof model_bytes, seed + 34);
    lookups = actor_number = stage = stop_before = sound_calls = 0;
    scan_case = linked;
    order[0] = linked ? 3 : (seed % SLOTS);
    order[1] = 95;
    actor_count = linked ? 2 : 1;
    for (unsigned i = 0; i < actor_count; ++i) {
        Obj8014A2E4 *a = &D_801202A0[order[i]];
        a->active = 424;
        a->probe_object_address = model_id[i];
        put16(a, 6, (u16)(0x8001 + 71 * i + seed));
        put16(a, 10, (u16)(0x7e05 - 101 * i - seed));
        put16(a, 14, (u16)(0xffffu - 179 * i - seed));
        put16(model_bytes[i], 18, (u16)(yaw ^ (0x8481u * i)));
    }
    if (linked) {
        put16(&D_801202A0[order[1]], 0x84, 3);
        put16(&D_801202A0[order[1]], 0x102, 1);
    }
    retain_actor_edits();
}

static void direct(unsigned seed, u16 yaw, s32 depth, u32 screen, u32 flags, unsigned stop)
{
    setup(0, seed, yaw);
    stop_before = stop;
    screen_word = screen;
    flag_word = flags;
    depth_word = depth;
    if (setjmp(held) == 0) {
        func_8017E910(active());
        assert(!stop_before && actor_number == 1 && stage == 0);
    } else {
        assert(stop_before == stage && actor_number == 0);
    }
    assert(lookups == 1);
    match_pool();
}

int main(void)
{
    static const u16 yaw[] = {0, 1, 0x7fff, 0x8000, 0xfffe, 0xffff};
    static const s32 depth[] = {INT32_MAX, -998, -1, 0, INT32_MIN, 335};
    static const u32 screen[] = {0x7fff8000u, 0xa1234567u, 0xfffffc19u, 0x7fff, 1, 0xffffffffu};
    static const u32 flags[] = {0, 7, 0x7fffffffu, 0x80000000u, 0xffff0000u, 0xffffffffu};
    for (unsigned i = 0; i < 6; ++i) {
        for (unsigned j = 0; j < 6; ++j) {
            direct(11 + 7 * i + 3 * j, yaw[i], depth[i], screen[i], flags[j], 0);
        }
        direct(7, yaw[i], depth[i], screen[i], flags[i], i + 1);
    }
    for (unsigned id = 0; id < 2; ++id) {
        setup(0, 11 + id, 0);
        active()->probe_object_address = id ? 0xff403150u : 0;
        retain_actor_edits();
        func_8017E910(active());
        assert(lookups == 1 && stage == 0 && !actor_number);
        match_pool();
    }
    setup(0, 17, 0xffe1);
    func_8017E910(NULL);
    assert(!lookups && !stage && !actor_number);
    match_pool();
#ifdef BFM_TEST_EMITTER_SCAN
    for (unsigned i = 1; i <= 2; ++i) {
        setup(i, 29 + i, 0x8e73);
        func_8017E794((MineEmitterActor *)&D_801202A0[order[1]]);
        assert(lookups == 2 && actor_number == 2 && stage == 0 && sound_calls == 1);
        match_pool();
    }
#endif
    return 0;
}
