#include "steam_knight_hit.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

enum { SLOTS = 96, SIZE = 268 };
static const u32 BASE = 0x91000000u;
static _Alignas(16) u8 pool[SLOTS][SIZE];
static int expected_slot, parent_slot, reply, request_kind;
static u32 request_selector;
static unsigned registrations;
extern SteamKnightPart *func_8012C658(int kind, u32 selector, int parent_address);

static u32 key(int slot) { return slot < 0 ? 0 : BASE + (u32)slot * SIZE; }
static u16 half(const u8 *p) { u16 v; memcpy(&v, p, 2); return v; }
static void put(u8 *p, u16 v) { memcpy(p, &v, 2); }
void *bfm_tower_actor_slots(void) { return pool; }
void *bfm_tower_spawn_pointer(u32 address)
{
    unsigned offset = address - BASE;
    assert(offset % SIZE == 0 && offset / SIZE < SLOTS);
    return pool[offset / SIZE];
}

s32 func_8012C890(void *data, u8 *slot, u8 *parent)
{
    const u8 *placement = data;
    assert(registrations++ == 0 && expected_slot >= 0 && slot == pool[expected_slot]);
    assert(parent == (parent_slot < 0 ? NULL : pool[parent_slot]));
    for (int i = 0; i < 3; ++i)
        assert(half(placement + 2*i) == (parent && half(parent) ? half(parent + 6 + 4*i) : 0));
    assert(half(placement + 6) == (u16)request_kind);
    assert(half(placement + 8) == (u16)request_selector);
    assert(half(placement + 10) == 0 && half(placement + 12) == 32767 && half(placement + 14) == 0);
    u32 user;
    memcpy(&user, placement + 16, sizeof user);
    assert(user == 0);
    slot[0x58] = 0xcd;
    if (parent)
        parent[0x7c] = 0xe1;
    return (s32)key(reply);
}

int main(void)
{
    assert(sizeof(SteamKnightPart) == SIZE);
    for (parent_slot = -1; parent_slot < SLOTS; ++parent_slot) {
        for (int layout = 0; layout < 6; ++layout) {
            memset(pool, 0, sizeof pool);
            expected_slot = -1;
            for (int i = 0; i < SLOTS; ++i) {
                int vacant = layout == 0 || (layout == 1 && i == parent_slot) ||
                    (layout == 2 && i == 95) || (layout == 3 && i == parent_slot + 1) ||
                    (layout == 4 && (i == 3 || i == 71 || i == 94));
                put(pool[i], vacant ? 0 : (u16)(i + 1));
                for (int j = 0; j < 3; ++j)
                    put(pool[i] + 6 + 4*j, (u16)(i * 897 + j * 83));
                if (i > parent_slot && vacant && expected_slot == -1)
                    expected_slot = i;
            }
            for (int outcome = 0; outcome < 3; ++outcome) {
                reply = outcome == 0 ? -1 : outcome == 1 ? expected_slot : 41;
                request_kind = outcome == 0 ? -43 : 0x17222;
                request_selector = 0xff938000u + (u32)(layout + outcome);
                registrations = 0;
                SteamKnightPart *result = func_8012C658(request_kind, request_selector, (int)key(parent_slot));
                assert(registrations == (unsigned)(expected_slot >= 0));
                if (expected_slot < 0 || reply < 0)
                    assert(result == NULL);
                else
                    assert((u8 *)result == pool[reply]);
                if (expected_slot >= 0)
                    assert(pool[expected_slot][0x58] == 0xcd);
                if (expected_slot >= 0 && parent_slot >= 0)
                    assert(pool[parent_slot][0x7c] == 0xe1);
            }
        }
    }
    return 0;
}
