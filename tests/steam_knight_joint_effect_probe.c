#include "steam_knight_hit.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

extern void func_80186F9C(SteamKnightPart *body, int joint);
s32 D_801E43A0;
static SteamKnightPart bodies[2];
static const u32 root_address = 0x801102A0u;
static unsigned calls, owner_queries;
static int success, use_self, last_kind, last_parent;
static u32 last_selector;

u32 bfm_steam_knight_effect_owner_address(const SteamKnightPart *part)
{
    assert(part == bodies);
    ++owner_queries;
    return root_address;
}

SteamKnightPart *func_8012C658(int kind, u32 selector, int parent)
{
    ++calls;
    last_kind = kind;
    last_parent = parent;
    last_selector = selector;
    bodies[0].linked_address += 0x100u;
    return success ? &bodies[use_self ? 0 : 1] : NULL;
}

int main(void)
{
    unsigned key, variant;
    const u32 initial_link = 0x80133422u;
    const u32 guard = 0xa943052fu;
    for (key = 0; key <= 0xffff; ++key) {
        for (variant = 0; variant < 16; ++variant) {
            unsigned index = key - 22, result = key;
            u32 modified;
            SteamKnightPart *child;
            int signed_key = variant & 8 ? (s32)(0xffff0000u | key) : (int)key;
            D_801E43A0 = (s32)(0x87008410u | ((variant & 1) << 1) | ((variant & 2) << 2));
            success = (variant & 4) != 0;
            use_self = (variant & 8) != 0;
            memset(bodies, 0, sizeof bodies);
            bodies[0].linked_address = initial_link;
            bodies[0].body_address = guard;
            bodies[1].body_address = guard;
            calls = owner_queries = 0;
            func_80186F9C(bodies, signed_key);
            assert(calls == (key < 32));
            assert(owner_queries == (key < 22));
            if (calls) {
                assert(last_kind == 81);
                assert((u32)last_parent == (key < 22 ? root_address : 0u));
                if (key >= 22) {
                    result = 0x100 | index;
                    if (index == 0) result = (variant & 1) ? 0x300 : 0x200;
                    if (index == 3) result = ((variant & 2) ? 0x500 : 0x400) | index;
                }
                assert(last_selector == result);
            }
            modified = calls ? initial_link + 0x100 : initial_link;
            assert(bodies[0].linked_address == modified);
            child = &bodies[use_self ? 0 : 1];
            assert(child->body_address == (key >= 22 && key < 32 && success ? modified : guard));
            assert(bodies[use_self ? 1 : 0].body_address == guard);
        }
    }
    return 0;
}
