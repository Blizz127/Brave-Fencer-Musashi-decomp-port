#include "mine_animation_apply.h"
#include "actor_position_updates.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

Obj8014A2E4 D_801202A0[96];
s16 D_801270C0;
extern void func_8012A988(Obj8014A2E4 *actor);

static u8 memory[1024], saved_memory[sizeof(memory)];
static const u32 memory_key = UINT32_C(0x945c3080);
static const u32 actor_key = UINT32_C(0xb2069400);
static u32 primary, secondary, target;
static unsigned limit, op_reads, reject_read, refusals, last_reason;

static u32 load(const u8 *p, unsigned width)
{
    unsigned i;
    u32 value = 0;
    for (i = 0; i < width; i++)
        value |= (u32)p[i] << (i * 8);
    return value;
}
static void store(u8 *p, unsigned width, u32 value)
{
    unsigned i;
    for (i = 0; i < width; i++)
        p[i] = (u8)(value >> (i * 8));
}
static u8 *view(u32 address, unsigned width)
{
    if (address >= memory_key && address - memory_key <= sizeof(memory) - width)
        return memory + (address - memory_key);
    if (address >= actor_key && address - actor_key <= sizeof(D_801202A0) - width)
        return (u8 *)D_801202A0 + (address - actor_key);
    return 0;
}
u8 *bfm_animation_word_view(u32 address, int writable)
{
    assert(writable == 0 || writable == 1);
    assert(!(address & 3u));
    return view(address, 4);
}
int bfm_animation_frame_operation(u32 address, s16 *result)
{
    u8 *p;
    u32 v;
    op_reads++;
    assert(!(address & 1u));
    p = view(address, 2);
    if (op_reads == reject_read || !p)
        return 0;
    v = load(p, 2);
    *result = (s16)((s32)(v ^ 0x8000u) - 0x8000);
    return 1;
}
u32 bfm_animation_command_budget(const Obj8014A2E4 *actor)
{
    assert(actor == D_801202A0);
    return limit;
}
void bfm_animation_command_refused(Obj8014A2E4 *actor, unsigned reason, u32 address)
{
    (void)address;
    assert(actor == D_801202A0 || !actor);
    refusals++;
    last_reason = reason;
}
static void record_at(u32 base, int i, u32 value, s16 op)
{
    u8 *p = view(base + (u32)i * 8u, 8);
    assert(p);
    store(p, 4, value);
    store(p + 4, 2, (u16)op);
}
static u8 *start(void)
{
    u8 *a = (u8 *)D_801202A0;
    memset(D_801202A0, 0, sizeof(D_801202A0));
    memset(memory, 0x35, sizeof(memory));
    primary = memory_key + 0x88;
    secondary = memory_key + 0x140;
    target = memory_key + 0x288;
    store(a, 2, 1);
    store(a + 0x20, 4, target);
    store(a + 0x72, 2, 0x4600);
    store(a + 0x90, 4, primary);
    store(a + 0x98, 2, 17);
    limit = 40;
    op_reads = reject_read = refusals = last_reason = 0;
    return a;
}
static void apply(void)
{
    func_8012AAAC(D_801202A0);
    assert(!refusals);
}
static void should_refuse(unsigned reason)
{
    Obj8014A2E4 original = D_801202A0[0];
    memcpy(saved_memory, memory, sizeof(memory));
    func_8012AAAC(D_801202A0);
    assert(refusals == 1 && reason == last_reason);
    assert(memcmp(&original, D_801202A0, sizeof(original)) == 0);
    assert(memcmp(saved_memory, memory, sizeof(memory)) == 0);
}
static void verify_delays(void)
{
    unsigned delay;
    for (delay = 0; delay < 32768; delay++) {
        u8 *a = start();
        u32 value = (delay * 73013u) ^ 0x5a381du;
        record_at(primary, 0, value, (s16)delay);
        apply();
        assert(op_reads == 2 && load(a + 0x98, 2) == delay);
        assert(load(view(target + 0x20, 4), 4) == value);
        assert(load(a + 0x72, 2) == 0x600);
    }
}
static void verify_controls(void)
{
    u8 *a = start();
    record_at(primary, 0, 0, -50);
    record_at(primary, 1, secondary, -3);
    record_at(secondary, 0, secondary + 0x60, -3);
    record_at(secondary + 0x60, 0, 0x7fb98a47u, 3);
    apply();
    assert(op_reads == 5 && load(a + 0x98, 2) == 3);
    assert(load(a + 0x9c, 4) == primary && load(a + 0x9a, 2) == 2);
    assert(load(a + 0x72, 2) == 0x2000);
    record_at(secondary + 0x60, 0, 0, -2);
    record_at(primary, 3, 0, -2);
    store(a + 0x9a, 2, 3);
    apply();
    assert(!load(a + 0x9c, 4) && !load(a + 0x98, 2));
    assert(load(a + 0x90, 4) == primary && load(a + 0x94, 4) == 3);
    a = start();
    record_at(primary, 1, secondary, -4);
    store(a + 0x94, 4, 1);
    record_at(secondary, 0, 0x4307u, 12);
    apply();
    assert(load(a + 0x98, 2) == 12 && load(a + 0x72, 2) == 0);
    a = start();
    record_at(primary, 1, 0, -1);
    record_at(primary, 0, 0x61773u, 9);
    store(a + 0x94, 4, 1);
    apply();
    assert(load(a + 0x72, 2) == 0x4600 && load(a + 0x94, 4) == 0);
    a = start();
    record_at(primary, 0, 0, -5);
    record_at(primary, -1, 0x7f11u, 8);
    apply();
    assert(load(a + 0x98, 2) == 8 && load(a + 0x94, 4) == UINT32_MAX);
    a = start();
    record_at(primary, 0, 0, -1);
    store(a + 0x9c, 4, secondary);
    store(a + 0x9a, 2, 0xffff);
    record_at(secondary, -1, 0x441597u, 15);
    apply();
    assert(load(a + 0x98, 2) == 15 && load(a + 0x94, 4) == UINT32_MAX);
    assert(!load(a + 0x9c, 4) && !load(a + 0x9a, 2));
    a = start();
    store(a + 0x9c, 4, secondary);
    record_at(primary, 0, 0, -3);
    record_at(primary, 1, 0, -27);
    apply();
    assert(load(a + 0x94, 4) == 1 && load(a + 0x9c, 4) == secondary);
}
static void verify_aliases(void)
{
    u8 *a = start();
    record_at(primary, 0, 0x7dfa81c3u, 11);
    store(a + 0x20, 4, primary + 4 - 0x20);
    apply();
    assert(load(a + 0x98, 2) == 0x81c3 && load(view(primary + 4, 4), 4) == 0x7dfa81c3u);
    a = start();
    record_at(primary, 0, 0x62c35c19u, 17);
    store(a + 0x20, 4, actor_key + 0x98 - 0x20);
    apply();
    assert(load(a + 0x98, 2) == 17 && load(a + 0x9a, 2) == 0x62c3);
}
static void verify_failures(void)
{
    u8 *a = start();
    record_at(primary, 0, 0, -1);
    limit = 3000;
    should_refuse(BFM_ANIMATION_COMMAND_LIMIT);
    assert(op_reads == 3000);
    a = start();
    record_at(primary, 0, primary, -4);
    should_refuse(BFM_ANIMATION_COMMAND_LIMIT);
    a = start();
    store(a + 0x94, 4, 0x20000000u);
    record_at(primary, 0, 0x5a21u, 6);
    apply();
    assert(load(a + 0x98, 2) == 6 && load(view(target + 0x20, 4), 4) == 0x5a21u);
    a = start();
    store(a + 0x90, 4, 0xfffffffcu);
    should_refuse(BFM_ANIMATION_BAD_FRAME);
    a = start();
    record_at(primary, 0, 0, 5);
    store(a + 0x20, 4, target + 1);
    should_refuse(BFM_ANIMATION_BAD_TARGET);
    a = start();
    record_at(primary, 0, 0x5753468au, 5);
    store(a + 0x20, 4, primary + 4 - 0x20);
    reject_read = 2;
    should_refuse(BFM_ANIMATION_BAD_FRAME);
    a = start();
    record_at(primary, 0, memory_key + sizeof(memory), -4);
    should_refuse(BFM_ANIMATION_BAD_FRAME);
    a = start();
    (void)a;
    func_8012AAAC(0);
    assert(refusals == 1 && last_reason == BFM_ANIMATION_BAD_ACTOR);
}

static void verify_prepass(void)
{
    u8 *a = start();
    store(a + 0x72, 2, 0x1200);
    store(a + 0x98, 2, 1);
    record_at(primary, 0, 1, 6);
    record_at(primary, 1, 2, 9);
    record_at(primary, 2, 0, -1);
    D_801270C0 = 0;
    func_8012A908();
    assert(!refusals && op_reads == 6);
    assert(load(a + 0x94, 4) == 1 && load(a + 0x98, 2) == 9);
    assert(load(view(target + 0x20, 4), 4) == 2);
}

int main(void)
{
    verify_delays();
    verify_controls();
    verify_aliases();
    verify_failures();
    verify_prepass();
    return 0;
}
