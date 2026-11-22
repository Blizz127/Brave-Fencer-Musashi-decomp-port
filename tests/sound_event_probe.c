/* Independently authored tests for the game event decoder. All records,
 * random seeds, channel states and handler results are synthetic. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "sound_events.h"

BfmSoundEntry D_8006451C[65536];
BfmSoundChannel D_800A46E8[16];
u16 D_8006A9F8[16];
u8 D_8006A988[8];
u32 D_800A4EEC;
s16 D_800A4EFC;
u8 D_800A4F17;
u8 D_800A4F1B;
u8 D_8006A980;
u8 D_8006A984;
u16 D_8006A982;

_Static_assert(sizeof(BfmSoundEntry) == 4, "event stride");
_Static_assert(offsetof(BfmSoundEntry, selector) == 3, "event selector offset");
_Static_assert(sizeof(BfmSoundChannel) == 84, "channel stride");
_Static_assert(offsetof(BfmSoundChannel, voice_used) == 14, "voice map offset");
_Static_assert(offsetof(BfmSoundChannel, pitch) == 32, "pitch offset");
_Static_assert(offsetof(BfmSoundChannel, pitch_target) == 40, "pitch target offset");
_Static_assert(offsetof(BfmSoundChannel, pitch_step) == 44, "pitch step offset");
_Static_assert(offsetof(BfmSoundChannel, pitch_timer) == 48, "pitch timer offset");
_Static_assert(offsetof(BfmSoundChannel, flags) == 55, "channel flag offset");
_Static_assert(offsetof(BfmSoundChannel, pitch_done) == 56, "pitch status offset");

enum { EVENT = 0x56f7, NESTED_EVENT = 0xe42a, REDIRECTED_EVENT = 0xafb9 };
enum Handler {
    HANDLE_32048, HANDLE_322A8, HANDLE_324A4, HANDLE_32774, HANDLE_34314,
    RESOLVE_REDIRECT, HANDLE_F714
};

typedef struct {
    enum Handler handler;
    u32 id;
    u32 argument;
    unsigned entry;
    s32 mode;
    u8 pause;
} ObservedCall;

static ObservedCall calls[5];
static unsigned call_count;
static u16 redirect_target;
static int change_pause_in_handler;
static const u8 initial_pause = 0x95;
static const s32 handler_results[] = { -180224317, 1907341, -217920113, 6201931, -19731201 };

static s32 record_call(enum Handler handler, u32 id, BfmSoundEntry *entry, u32 argument, s32 mode)
{
    assert(call_count < sizeof(calls) / sizeof(calls[0]));
    assert(entry >= D_8006451C && entry < D_8006451C + 65536);
    calls[call_count++] = (ObservedCall) {
        handler, id, argument, (unsigned)(entry - D_8006451C), mode, D_800A4F17
    };
    if (handler > HANDLE_34314) {
        return 0;
    }
    if (change_pause_in_handler) {
        D_800A4F17 = 0x73;
    }
    return handler_results[handler];
}

s32 func_80032048(s32 id, BfmSoundEntry *entry, u16 argument)
{
    return record_call(HANDLE_32048, (u32)id, entry, argument, 0);
}

s32 func_800322A8(s32 id, BfmSoundEntry *entry, u16 argument)
{
    return record_call(HANDLE_322A8, (u32)id, entry, argument, 0);
}

s32 func_800324A4(s32 id, BfmSoundEntry *entry, u16 argument, s32 mode)
{
    return record_call(HANDLE_324A4, (u32)id, entry, argument, mode);
}

s32 func_80032774(s32 id, BfmSoundEntry *entry, u16 argument)
{
    return record_call(HANDLE_32774, (u32)id, entry, argument, 0);
}

s32 func_80034314(u32 id, void *entry, u32 argument)
{
    return record_call(HANDLE_34314, id, entry, argument, 0);
}

u16 func_8002F4E4(BfmSoundEntry *entry)
{
    record_call(RESOLVE_REDIRECT, 0, entry, 0, 0);
    return redirect_target;
}

void func_8002F714(u16 selector, s32 argument)
{
    record_call(HANDLE_F714, selector, D_8006451C, (u32)argument, 0);
}

static void expect_call(unsigned index, enum Handler handler, u32 id, unsigned entry,
                        u32 argument, s32 mode, u8 pause)
{
    assert(index < call_count);
    const ObservedCall *call = &calls[index];
    assert(call->handler == handler);
    assert(call->id == id);
    assert(call->entry == entry);
    assert(call->argument == argument);
    assert(call->mode == mode);
    assert(call->pause == pause);
}

static void reset(void)
{
    for (unsigned i = 0; i < 65536; i++) {
        D_8006451C[i] = (BfmSoundEntry) { 1, { 0x37, 0x91 }, 0xf3 };
    }
    memset(D_800A46E8, 0, sizeof(D_800A46E8));
    for (unsigned i = 0; i < 16; i++) {
        D_8006A9F8[i] = (u16)(0x920a + i * 173);
    }
    for (unsigned i = 0; i < 8; i++) {
        D_8006A988[i] = (u8)(11 + 9 * i);
    }
    call_count = 0;
    redirect_target = 0;
    change_pause_in_handler = 0;
    D_800A4EEC = 0xdccbe137;
    D_800A4EFC = 0;
    D_800A4F17 = initial_pause;
    D_800A4F1B = 1;
    D_8006A980 = 0;
    D_8006A982 = 54321;
    D_8006A984 = 21;
}

static void set_selector(unsigned entry, u8 selector)
{
    D_8006451C[entry].kind = 2;
    D_8006451C[entry].selector = selector;
}

static void expect_simple_sound(unsigned index, u16 id)
{
    expect_call(index, HANDLE_32048, id, id, 0, 0, 1);
}

static u32 next_seed(u32 seed)
{
    return (u32)((uint64_t)seed * 1103515245u + 12345u);
}

static u32 seed_for_bucket(unsigned buckets, unsigned bucket)
{
    for (u32 seed = 0xeff31175u;; seed++) {
        if (((next_seed(seed) >> 16) & (buckets - 1)) == bucket) {
            return seed;
        }
    }
}

static u16 expected_random_sound(unsigned selector, unsigned bucket)
{
    if (selector == 7) {
        return D_8006A9F8[bucket];
    }
    switch (bucket) {
    case 0:
    case 3:
        return selector == 0 ? 1660 : 2813;
    case 1:
    case 2:
        return selector == 0 ? 1673 : 2815;
    case 4:
        return selector == 0 ? 1659 : 2814;
    case 5:
        return selector == 0 ? 1659 : 2815;
    default:
        return 0;
    }
}

static void test_seeded_selection(void)
{
    const unsigned selectors[] = { 0, 4, 7 };
    for (unsigned k = 0; k < sizeof(selectors) / sizeof(selectors[0]); k++) {
        unsigned selector = selectors[k];
        unsigned buckets = selector == 7 ? 16 : 8;
        for (unsigned bucket = 0; bucket < buckets; bucket++) {
            reset();
            set_selector(EVENT, (u8)selector);
            u32 seed = seed_for_bucket(buckets, bucket);
            u16 expected = expected_random_sound(selector, bucket);
            D_800A4EEC = seed;
            assert(func_8002DC68(EVENT, 0xfe35) == 0);
            assert(D_800A4EEC == next_seed(seed));
            assert(D_800A4F17 == initial_pause);
            assert(call_count == (unsigned)(expected != 0));
            if (expected) {
                expect_simple_sound(0, expected);
            }
        }
    }
}

static void test_fixed_and_unknown_selectors(void)
{
    for (unsigned value = 0; value < 256; value++) {
        if (value == 0 || value == 4 || value == 7) {
            continue;
        }
        reset();
        set_selector(EVENT, (u8)value);
        u32 seed_before = D_800A4EEC;
        change_pause_in_handler = 1;
        assert(func_8002DC68(EVENT, 0xd741) == 0);
        assert(D_800A4EEC == seed_before);
        assert(D_800A4F17 == initial_pause);
        switch (value) {
        case 1:
            assert(call_count == 1);
            expect_simple_sound(0, 0xd741);
            break;
        case 5:
        case 6:
            assert(call_count == 2);
            expect_simple_sound(0, 2822);
            expect_simple_sound(1, value == 5 ? 2823 : 3168);
            break;
        default:
            assert(call_count == 0);
            break;
        }
    }
}

static void test_recursive_dispatch_and_direct_selector_argument(void)
{
    reset();
    set_selector(EVENT, 1);
    set_selector(NESTED_EVENT, 6);
    change_pause_in_handler = 1;
    assert(func_8002DC68(EVENT, NESTED_EVENT) == 0);
    assert(D_800A4F17 == initial_pause);
    assert(call_count == 2);
    expect_simple_sound(0, 2822);
    expect_simple_sound(1, 3168);

    reset();
    set_selector(EVENT, 1);
    func_8002F248(&D_8006451C[EVENT], (s32)0x9f437ab2u);
    assert(D_800A4F17 == initial_pause);
    assert(call_count == 1);
    expect_simple_sound(0, 0x7ab2);
}

static void prepare_pitch_channels(unsigned matched_slot)
{
    for (unsigned i = 0; i < 16; i++) {
        BfmSoundChannel *channel = &D_800A46E8[i];
        memset(channel, (int)(0x90 + i), sizeof(*channel));
        channel->kind = 0;
        channel->id = 2396;
        if (i >= 8) {
            channel->pitch = i & 1 ? 0xfffffff0u - i : i + 9;
            channel->flags = 0xa4;
        }
    }
    /* A matching ID in the voice bank must never satisfy the eight-slot lookup. */
    D_800A46E8[8].kind = 1;
    if (matched_slot < 8) {
        BfmSoundChannel *slot = &D_800A46E8[matched_slot];
        slot->kind = 0xffc1;
        slot->sub_id = 0xfdd4;
        for (unsigned i = 0; i < 8; i++) {
            slot->voice_used[i] = i % 3 ? 0xa5 : 0;
        }
    }
}

static void test_pitch_delta_and_unselected_voices(void)
{
    for (unsigned up = 0; up <= 1; up++) {
        for (unsigned slot = 0; slot <= 8; slot++) {
            BfmSoundChannel expected[16];
            reset();
            set_selector(EVENT, (u8)(2 + up));
            prepare_pitch_channels(slot);
            memcpy(expected, D_800A46E8, sizeof(expected));
            for (unsigned i = 0; i < 8; i++) {
                if (slot == 8 || !expected[slot].voice_used[i]) {
                    continue;
                }
                BfmSoundChannel *voice = &expected[8 + i];
                voice->pitch_target = voice->pitch + (up ? 256u : 0xffffff00u);
                voice->pitch_timer = 0;
                voice->pitch_done = 0;
                voice->pitch_step = up ? 24 : -24;
                voice->flags |= 1;
            }
            assert(func_8002DC68(EVENT, 0) == 0);
            assert(memcmp(D_800A46E8, expected, sizeof(expected)) == 0);
            assert(call_count == 0);
            assert(D_800A4F17 == initial_pause);
        }
    }
}

static void test_slot_id_and_optional_sub_id(void)
{
    reset();
    for (unsigned i = 0; i < 8; i++) {
        BfmSoundChannel *slot = &D_800A46E8[i];
        slot->id = 0xabd4;
        slot->kind = 0x8041;
        slot->sub_id = (u16)(0xfd80 + i);
    }
    for (unsigned i = 0; i < 8; i++) {
        u32 key = ((0xfd80u + i) << 16) | 0xabd4;
        assert(func_800331D4(key) == (s32)i + 1);
    }
    assert(func_800331D4(0xabd4) == 1);
    assert(func_800331D4(0xffffabd4) == 0);
    assert(func_800331D4(0xfb33) == 0);
    D_800A46E8[0].kind = 0x8042;
    assert(func_800331D4(0xabd4) == 2);
}

static enum Handler simple_handler(unsigned kind)
{
    switch (kind) {
    case 1: return HANDLE_32048;
    case 5: return HANDLE_34314;
    case 7:
    case 9: return HANDLE_324A4;
    default: return HANDLE_32774;
    }
}

static u32 forced_sub_id(unsigned kind)
{
    switch (kind) {
    case 13: return 2292;
    case 14: return 2698;
    case 15: return 2198;
    case 16: return 2941;
    default: return 1267;
    }
}

static void test_dispatch_kinds(void)
{
    const u32 id = 0x93240000u | EVENT;
    const u16 argument = 0xfff6;
    for (unsigned kind = 0; kind < 64; kind++) {
        reset();
        D_8006451C[EVENT].kind = (u8)(0xc0 | kind);
        s32 result = func_8002DC68((s32)id, argument);
        s32 expected_result = 0;
        unsigned expected_calls = 0;
        enum Handler handler;
        switch (kind) {
        case 1:
        case 5:
        case 7:
        case 8:
        case 9:
        case 10:
            if (kind == 10) {
                expect_call(expected_calls++, HANDLE_322A8, id, EVENT, argument, 0, 1);
            }
            handler = simple_handler(kind);
            expect_call(expected_calls++, handler, id, EVENT, argument, kind == 7, 1);
            expected_result = handler_results[handler];
            break;
        case 6:
            expect_call(expected_calls++, RESOLVE_REDIRECT, 0, EVENT, 0, 0, initial_pause);
            break;
        case 11:
            assert(D_8006A980 == 5);
            assert(D_8006A982 == 65);
            break;
        case 12:
            assert(D_8006A980 == 8);
            assert(D_8006A984 == 118);
            break;
        case 13:
        case 14:
        case 15:
        case 16:
        case 18: {
            u32 high = forced_sub_id(kind);
            expect_call(expected_calls++, HANDLE_32048, high << 16 | EVENT, EVENT, 65535, 0, 1);
            expected_result = handler_results[HANDLE_32048];
            break;
        }
        case 17:
            expect_call(expected_calls++, HANDLE_F714, 0xf3, 0, argument, 0, 1);
            break;
        }
        assert(result == expected_result);
        assert(call_count == expected_calls);
        assert(D_800A4F17 == initial_pause);
    }
}

static void test_redirect_and_play_gates(void)
{
    const u32 id = 0x93240000u | EVENT;
    reset();
    D_8006451C[EVENT].kind = 6;
    D_8006451C[REDIRECTED_EVENT].kind = 9;
    redirect_target = REDIRECTED_EVENT;
    assert(func_8002DC68((s32)id, 0xff87) == handler_results[HANDLE_324A4]);
    assert(call_count == 2);
    expect_call(0, RESOLVE_REDIRECT, 0, EVENT, 0, 0, initial_pause);
    expect_call(1, HANDLE_324A4, id, REDIRECTED_EVENT, 0xff87, 0, 1);
    assert(D_800A4F17 == initial_pause);

    reset();
    D_800A4EFC = -9;
    assert(func_8002DC68((s32)id, 1) == 0);
    assert(call_count == 0);
    assert(D_800A4F17 == initial_pause);

    reset();
    D_800A4F1B = 0;
    assert(func_8002DC68((s32)id, 1) == 0);
    assert(call_count == 0);
    D_8006451C[EVENT].kind = 0x81;
    assert(func_8002DC68((s32)id, 1) == handler_results[HANDLE_32048]);
    expect_call(0, HANDLE_32048, id, EVENT, 1, 0, 1);
    assert(call_count == 1);
    assert(D_800A4F17 == initial_pause);
}

static void test_accumulator_wrap_and_maximum(void)
{
    reset();
    D_8006451C[EVENT].kind = 11;
    D_8006A980 = 4;
    D_8006A982 = 65530;
    assert(func_8002DC68(EVENT, 0xfffa) == 0);
    assert(D_8006A980 == 6);
    assert(D_8006A982 == (u16)(65530 + 29 - 2));

    reset();
    D_8006451C[EVENT].kind = 12;
    D_8006A984 = 103;
    assert(func_8002DC68(EVENT, 0x8002) == 0);
    assert(D_8006A984 == 103);
    assert(D_8006A980 == 8);
}

int main(void)
{
#ifdef BFM_TEST_HIGH_ADDRESSES
    /* Optional Linux LP64 PIE profile for lossless forwarding of host pointers. */
    assert((uintptr_t)D_8006451C > UINT32_MAX);
    assert((uintptr_t)D_800A46E8 > UINT32_MAX);
#endif
    test_seeded_selection();
    test_fixed_and_unknown_selectors();
    test_recursive_dispatch_and_direct_selector_argument();
    test_pitch_delta_and_unselected_voices();
    test_slot_id_and_optional_sub_id();
    test_dispatch_kinds();
    test_redirect_and_play_gates();
    test_accumulator_wrap_and_maximum();
    return 0;
}
