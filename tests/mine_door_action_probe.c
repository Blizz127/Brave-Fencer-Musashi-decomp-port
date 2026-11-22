#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "mine_door_action.h"

u32 D_800AE6B0;
void (*D_80180D78[37])(MineDoorActionPlayer *);
static MineDoorActionPlayer *expected_player;
static unsigned int called;
static unsigned int expected_slot;

static void record_action(MineDoorActionPlayer *player) {
    assert(player == expected_player);
    assert(player->action == expected_slot);
    ++called;
    player->action = 19;
    D_800AE6B0 = 0x40000001;
}

static void unexpected_action(MineDoorActionPlayer *player) {
    (void)player;
    assert(0);
}

static void check(u32 pending, u16 action, unsigned int expected_calls) {
    MineDoorActionPlayer player;
    unsigned int slot;

    memset(&player, 0x63, sizeof(player));
    assert(offsetof(MineDoorActionPlayer, action) == 0x16c);
    for (slot = 0; slot < 37; ++slot) {
        D_80180D78[slot] = unexpected_action;
    }
    if (action > 0 && action <= 37) {
        D_80180D78[action - 1] = record_action;
    }
    player.action = action;
    D_800AE6B0 = pending;
    expected_player = &player;
    expected_slot = action;
    called = 0;
    func_80150B28(&player);
    assert(called == expected_calls);
    assert(player.action == 0);
    assert(D_800AE6B0 == (expected_calls ? 0x40000001u : pending));
    for (slot = 0; slot < sizeof(player.reserved); ++slot) {
        assert(player.reserved[slot] == 0x63);
    }
}

int main(void) {
    unsigned int action;
    for (action = 1; action <= 37; ++action) {
        check(0, action, 1);
        check(0x7f000000, action, 1);
        check(0x80000000, action, 0);
        check(0x00800000, action, 0);
        check(1, action, 0);
    }
    check(0, 0, 0);
    check(0, 38, 0);
    check(0, 0xffff, 0);
    return 0;
}
