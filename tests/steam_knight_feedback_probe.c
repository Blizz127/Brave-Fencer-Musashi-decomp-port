#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "steam_knight_hit.h"
#include "steam_knight_effect.h"

SteamKnightEffect D_8011D030[30];
s16 D_80126B5E, D_80126B62, D_80126B66;

typedef struct {
    int kind;
    u32 owner;
    int x, y, z, mode, amount;
} Submission;

enum Call { CLEANUP, OWNER, SUBMIT };

static enum Call calls[3];
static unsigned call_count;
static const SteamKnightPart *bound_part;
static u32 bound_key;
static SteamKnightEffect *cleaned;
static Submission submitted;
static s16 replacement_position[3];

static void record(enum Call call)
{
    assert(call_count < sizeof calls / sizeof calls[0]);
    calls[call_count++] = call;
}

u32 bfm_steam_knight_effect_owner_address(const SteamKnightPart *part)
{
    assert(part == bound_part);
    record(OWNER);
    return bound_key;
}

void func_80146C3C(SteamKnightEffect *slot)
{
    record(CLEANUP);
    cleaned = slot;
    slot->kind = 0;
    D_80126B5E = replacement_position[0];
    D_80126B62 = replacement_position[1];
    D_80126B66 = replacement_position[2];
}

SteamKnightEffect *func_80146A6C(int kind, u32 owner, int x, int y,
                                 s16 z, int mode, int amount)
{
    record(SUBMIT);
    submitted = (Submission){kind, owner, x, y, z, mode, amount};
    return NULL;
}

static void set_axis(SteamKnightPart *part, unsigned axis, s16 value)
{
    part->hit_position.bytes[axis * 2] = (u8)value;
    part->hit_position.bytes[axis * 2 + 1] = (u8)((u16)value >> 8);
}

static void run(int damage, int first, u16 kind, const s16 position[3],
                int wrapped_y)
{
    SteamKnightPart part, saved;
    SteamKnightEffect expected_pool[30];
    const s16 initial[] = {840, 125, -76};
    const s16 contact[] = {position[2], position[0], position[1]};
    unsigned i;

    memset(&part, 0x5a, sizeof part);
    for (i = 0; i < 3; ++i) set_axis(&part, i, contact[i]);
    memcpy(&saved, &part, sizeof saved);
    bound_part = &part;
    bound_key = (u32)(uintptr_t)&part == 0x23451000u ? 0x23452000u : 0x23451000u;
#if UINTPTR_MAX > UINT32_MAX
    assert((uintptr_t)&part > UINT32_MAX);
#endif
    memset(D_8011D030, 0x53, sizeof D_8011D030);
    for (i = 0; i < 30; ++i) D_8011D030[i].kind = 0x109;
    if (first < 30) D_8011D030[first].kind = kind;
    if (first < 29) D_8011D030[first + 1].kind = 0;
    memcpy(expected_pool, D_8011D030, sizeof expected_pool);
    D_80126B5E = initial[0];
    D_80126B62 = initial[1];
    D_80126B66 = initial[2];
    memcpy(replacement_position, position, sizeof replacement_position);
    call_count = 0;
    cleaned = NULL;

    func_8016AA50(&part, damage);

    if (damage < 0 && first == 30) {
        assert(call_count == 0);
    } else {
        if (damage < 0) {
            assert(call_count == 3 && calls[0] == CLEANUP);
            assert(cleaned == &D_8011D030[first]);
            expected_pool[first].kind = 0;
        } else {
            assert(call_count == 2 && cleaned == NULL);
        }
        assert(calls[call_count - 2] == OWNER && calls[call_count - 1] == SUBMIT);
        assert(submitted.kind == 29 && submitted.owner == bound_key);
        assert(submitted.x == (damage < 0 ? position[0] : contact[0]));
        assert(submitted.y == (damage < 0 ? wrapped_y : contact[1]));
        assert(submitted.z == (damage < 0 ? position[2] : contact[2]));
        assert(submitted.mode == (damage < 0 ? 7 : 0));
        assert(submitted.amount == (damage < 0 ? 0 : damage));
    }
    if (cleaned) {
        assert(D_80126B5E == position[0] && D_80126B62 == position[1]);
        assert(D_80126B66 == position[2]);
    } else {
        assert(D_80126B5E == initial[0] && D_80126B62 == initial[1]);
        assert(D_80126B66 == initial[2]);
    }
    assert(memcmp(&part, &saved, sizeof part) == 0);
    assert(memcmp(expected_pool, D_8011D030, sizeof expected_pool) == 0);
}

int main(void)
{
    static const struct {
        s16 point[3];
        int shifted_y;
    } positions[] = {
        {{-32768, -32768, 32767}, 32736}, {{945, -32737, -754}, 32767},
        {{-32, -32736, 0}, -32768}, {{32767, 32767, -32768}, 32735}
    };
    const int damages[] = {INT_MIN, -1, 0, 11, INT_MAX};
    const u16 kinds[] = {0, 3, 9};
    unsigned p, d, k;
    int first;

    _Static_assert(sizeof(SteamKnightEffect) == 88, "pool stride");
    _Static_assert(offsetof(SteamKnightPart, hit_position) == 124, "contact position");
    for (p = 0; p < sizeof positions / sizeof positions[0]; ++p) {
        for (d = 0; d < sizeof damages / sizeof damages[0]; ++d) {
            for (k = 0; k < sizeof kinds / sizeof kinds[0]; ++k) {
                for (first = 0; first <= 30; ++first) {
                    run(damages[d], first, kinds[k], positions[p].point, positions[p].shifted_y);
                }
            }
        }
    }
    return 0;
}
