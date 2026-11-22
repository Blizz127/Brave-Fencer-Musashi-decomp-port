#include "mine_camera_frame.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define KNOWN_HOOK UINT32_C(0xf2708324)
#define MISSING_HOOK UINT32_C(0x964a0730)
#define PRIOR_KEY UINT32_C(0xa7a47540)

_Static_assert(sizeof(FacingActor) == 0xa4, "camera stride");
_Static_assert(offsetof(FacingActor, camera) == 0x3c, "camera components");

FacingActor D_80126948, D_80126E60, D_80126F04, D_80126FA8, D_80126DB8;
u8 D_801150D6, D_80127504;
u8 D_800AF630[64], D_800AE688[32];
u32 D_801151D4;
static FacingActor *const cameras[] = { &D_80126948, &D_80126E60, &D_80126F04, &D_80126FA8 };
static const u32 keys[] = { 0x80126948, 0x80126e60, 0x80126f04, 0x80126fa8 };
static FacingActor former[4];
static FacingActor *selected;
static u8 selector, last_timer;
static int smoothing;
static u32 requested_hook, selected_key;
static unsigned prepared, resolved, hooked, parameter_calls, smooth_calls, views, refusals;
static u32 target[7], previous[7], result[7], view_offset[7];

static u32 word(const void *base, unsigned offset)
{
    u32 bits;
    memcpy(&bits, (const u8 *)base + offset, sizeof bits);
    return bits;
}

static void set_word(void *base, unsigned offset, u32 bits)
{
    memcpy((u8 *)base + offset, &bits, sizeof bits);
}

static s16 low_half(u32 value)
{
    u32 low = value & 0xffffu;
    return (s16)(low <= INT16_MAX ? (s32)low : (s32)low - 65536);
}

static void set_component(FacingActor *camera, unsigned i, u32 value)
{
    set_word(camera, 0x3c + 4 * i, value);
}

static void set_hook(FacingActor *camera, u32 key)
{
    for (unsigned j = 0; j < 4; ++j)
        camera->callback_address[j] = (u8)(key >> (8 * j));
}

void bfm_camera_frame_prepare(void)
{
    assert(!prepared && !parameter_calls && !views);
    ++prepared;
    D_801150D6 = selector;
}

static void camera_callback(FacingActor *camera)
{
    assert(camera == selected && resolved == 1 && !hooked && !parameter_calls);
    ++hooked;
    D_801150D6 = selector == 1 ? 2 : 1;
    set_word(camera, 8, 0x8000bc23u);
    D_80127504 = smoothing ? 0 : 0xfc;
}

SceneCameraHandler bfm_camera_frame_hook(u32 key)
{
    assert(prepared == 1 && !parameter_calls && key == requested_hook && key != 0);
    ++resolved;
    return key == KNOWN_HOOK ? camera_callback : NULL;
}

void bfm_camera_frame_refused(u32 key)
{
    assert(key == MISSING_HOOK && resolved == 1 && !parameter_calls);
    ++refusals;
}

void bfm_camera_frame_set_parameter(s32 value)
{
    assert(prepared == 1 && !parameter_calls && !views);
    assert((u32)value == (hooked ? 0x8000bc23u : 0x79b48203u));
    ++parameter_calls;
    D_801150D6 = selector == 3 ? 2 : 3;
    D_80127504 = smoothing ? 0x85 : 0;
}

s32 bfm_camera_frame_ease(s16 current, s16 desired, s16 divisor, s16 minimum)
{
    unsigned i = smooth_calls++;
    assert(parameter_calls == 1 && smoothing && i < 7 && !views);
    assert(current == low_half(previous[i]) && desired == low_half(target[i]));
    assert(divisor == 2 && minimum == 1);
    if (i)
        assert(word(&D_80126DB8, 0x3c + 4 * (i - 1)) == result[i - 1]);
    if (i < 6) {
        /* The next call reads both words again and takes their low signed halves. */
        previous[i+1] ^= 0x035a9291u + 19u * i;
        target[i+1] ^= 0xde643c17u + 47u * i;
        set_component(&D_80126DB8, i+1, previous[i+1]);
        set_component(selected, i+1, target[i+1]);
    } else {
        last_timer = selector % 2 ? 0 : 27;
        D_80127504 = last_timer;
    }
    s16 step = low_half((u32)(u16)current * 983u - (u32)(u16)desired + i);
    if (selector == 0)
        step = (i & 1u) ? INT16_MIN : INT16_MAX;
    result[i] = (u32)(s32)step;
    u32 upper = ((i & 1u) ? 0x2971u : 0xa17fu) + selector;
    u32 raw_word = (upper << 16) | (u16)step;
    s32 returned;
    memcpy(&returned, &raw_word, sizeof returned);
    assert((u32)returned != result[i]);
    return returned;
}

void bfm_camera_frame_submit_view(s32 *view)
{
    assert(!views && parameter_calls == 1 && (u8 *)view == (u8 *)selected + 0x5c);
    assert(smooth_calls == (smoothing ? 7u : 0u));
    assert(D_80127504 == (smoothing ? (u8)(last_timer - 1u) : 0));
    assert(word(selected, 0x78) == word(selected, 0x58));
    assert(D_801151D4 == PRIOR_KEY && D_800AF630[0x18] == 0x97);
    for (unsigned j = 0; j < 7; ++j) {
        u32 position = smoothing ? result[j] : target[j];
        assert(word(&D_80126DB8, 0x3c + 4*j) == position);
        assert((u32)view[j] == position + view_offset[j]);
    }
    ++views;
    for (unsigned j = 0; j < sizeof D_800AE688; ++j)
        D_800AE688[j] = (u8)(j * 71u + 33u + selector);
    D_801151D4 = 0x9136a2feu;
}

static void run(u8 wanted, int ease, u32 hook)
{
    unsigned bank = (wanted >= 1 && wanted <= 3) ? wanted : 0;
    selector = wanted;
    smoothing = ease;
    requested_hook = hook;
    selected = cameras[bank];
    selected_key = keys[bank];
    prepared = resolved = hooked = parameter_calls = smooth_calls = views = refusals = 0;
    D_801150D6 = (u8)(wanted ^ 0xa2);
    D_80127504 = ease ? 0 : 0x6a;
    D_801151D4 = PRIOR_KEY;
    memset(D_800AF630, 0x97, sizeof D_800AF630);
    memset(D_800AE688, 0x32, sizeof D_800AE688);
    memset(&D_80126DB8, 0x69, sizeof D_80126DB8);
    for (unsigned b = 0; b < 4; ++b)
        memset(cameras[b], (int)(0x91+b), sizeof *cameras[b]);
    set_word(selected, 8, 0x79b48203u);
    set_word(selected, 0x58, 0xb70f5b39u ^ wanted);
    set_hook(selected, hook);
    for (unsigned j = 0; j < 7; ++j) {
        previous[j] = 0xed340137u * (j + 1u) + wanted;
        target[j] = 0x7fffffffu - 3u * j - wanted;
        view_offset[j] = 0x7fffff19u - j;
        if (j & 1u) {
            target[j] = 0x80000003u + wanted + j;
            view_offset[j] = 0x80000182u + j;
        }
        set_component(selected, j, target[j]);
        set_component(&D_80126DB8, j, previous[j]);
        set_word(selected, 0x7c + 4*j, view_offset[j]);
    }
    for (unsigned b = 0; b < 4; ++b)
        former[b] = *cameras[b];
    FacingActor old_current = D_80126DB8;

    func_80129CF8();

    assert(prepared == 1 && resolved == (unsigned)(hook != 0));
    assert(hooked == (unsigned)(hook == KNOWN_HOOK));
    if (hook == MISSING_HOOK) {
        assert(refusals == 1 && !parameter_calls && !smooth_calls && !views);
        assert(D_801151D4 == PRIOR_KEY && !memcmp(&D_80126DB8, &old_current, sizeof old_current));
    } else {
        assert(!refusals && parameter_calls == 1 && views == 1 && D_801151D4 == selected_key);
        assert(!memcmp(D_800AF630 + 0x18, D_800AE688, sizeof D_800AE688));
    }
    for (unsigned j = 0; j < sizeof D_800AF630; ++j)
        if (hook == MISSING_HOOK || j < 0x18 || j >= 0x38)
            assert(D_800AF630[j] == 0x97);
    for (unsigned b = 0; b < 4; ++b)
        if (b != bank || hook == MISSING_HOOK)
            assert(!memcmp(cameras[b], &former[b], sizeof former[b]));
}

int main(void)
{
    if ((uintptr_t)&D_80126DB8 <= UINT32_MAX || (uintptr_t)D_800AE688 <= UINT32_MAX)
        return 77;
    for (unsigned b = 0; b < 4; ++b)
        if ((uintptr_t)cameras[b] <= UINT32_MAX)
            return 77;
    for (unsigned i = 0; i <= UINT8_MAX; ++i) {
        for (int ease = 0; ease <= 1; ++ease) {
            run((u8)i, ease, KNOWN_HOOK);
            run((u8)i, ease, 0);
        }
    }
    run(2, 0, MISSING_HOOK);
    run(0xf9, 1, MISSING_HOOK);
    return 0;
}
