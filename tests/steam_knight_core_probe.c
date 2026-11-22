#include "steam_knight_core.h"
#include <assert.h>
#include <string.h>


u16 D_801E4BBC;
s16 D_80195B24[3];
u8 D_801E44E0[1], D_801963D0[1];
u32 D_801D6D94;

enum { BODY = 0x1200, LINK = 0x1450, MODEL = 0x1700, OTHER = 0x1900 };
enum { DISPLAY = 0x80196208u };
static _Alignas(4) u8 memory[0x2400], display_area[0x100];
static SteamKnightPart core;
static int events[5], used, accepted;
static int redirect;

static void store(void *dst, u32 value) { memcpy(dst, &value, sizeof value); }
static u32 word(const void *src) { u32 v; memcpy(&v, src, sizeof v); return v; }
static u8 *data(u32 offset)
{
    if (offset == DISPLAY)
        return display_area;
    assert(offset < sizeof memory - 0x100);
    return memory + offset;
}

SteamKnightPart *bfm_steam_knight_break_actor(u32 address)
{
    return (SteamKnightPart *)data(address);
}

u8 *bfm_steam_knight_core_model(u32 address) { return data(address); }

static void event(int id)
{
    assert(used < 5);
    events[used++] = id;
}

void func_8016AA50(SteamKnightPart *actor, int amount)
{
    SteamKnightPart *body = bfm_steam_knight_break_actor(BODY);
    assert(actor == &core && body->phase_flags & 0x10u);
    assert(body->damage == 5 + amount);
    accepted = amount;
    event(1);
    if (redirect)
        body->linked_address = DISPLAY;
}

void func_801858C4(void) { event(2); }

void func_80132288(void *effect, const void *descriptor, u32 value)
{
    assert(effect == D_801E44E0 && descriptor == D_801963D0 && value == D_801D6D94);
    assert(((u8 *)&core)[0x34] == 0 && ((u8 *)&core)[2] == 3);
    event(3);
}

void func_8002D4C8(u16 command, u16 argument)
{
    assert(command == 1266 && !argument);
    event(4);
}

static void check_case(u32 flags, unsigned round, int layout, int retarget)
{
    SteamKnightPart *body = bfm_steam_knight_break_actor(BODY);
    const int before = 100;
    static const int closed[] = {3, 2, 4};
    static const int open[] = {1, 2};
    memset(memory, 0x91, sizeof memory);
    memset(display_area, 0x91, sizeof display_area);
    memset(&core, 0, sizeof core);
    core.flags = 0x8801;
    core.damage = 11;
    core.body_address = BODY;
    body->phase_flags = flags;
    body->hp = before;
    body->damage = 5;
    body->linked_address = LINK;
    store(memory + BODY + 0x20, layout == 1 ? BODY + 0xcc - 0x80 :
                                layout == 2 ? LINK + 0x20 - 0x80 : MODEL);
    store(memory + LINK + 0x20, OTHER);
    store(display_area + 0x20, OTHER);
    D_801D6D94 = 0x731403edu;
    D_801E4BBC = (u16)round;
    D_80195B24[round] = 188;
    redirect = retarget;
    accepted = 12345;
    used = 0;
    func_80184358(&core);
    assert(core.flags == 0x8800 && body->hp == before);
    if (!(flags & 2u)) {
        assert(!core.reaction && core.field_ae == -1);
        assert(used == 3 && !memcmp(events, closed, sizeof closed));
        assert(body->phase_flags == flags && body->damage == 5);
        return;
    }
    assert(core.reaction == 1 && core.timer == 8);
    if (flags & 4u) {
        assert(used == 1 && events[0] == 2 && accepted == 12345);
        assert(body->phase_flags == flags && body->damage == 5);
        return;
    }
    int bypass = (flags & (round == 2 ? 0x800u : 0x80000u)) != 0;
    int expected = bypass ? 11 : round == 2 ? -33 : 1;
    assert(accepted == expected && body->damage == 5 + expected);
    assert(used == 2 && !memcmp(events, open, sizeof open));
    assert(body->phase_flags == (flags | 0x10u));
    assert(memory[BODY + 0xde] == 8 && memory[BODY + 0xdf] == 0);
    if (layout == 0)
        assert(word(memory + MODEL + 0x80) == DISPLAY);
    if (layout == 1) {
        assert(body->linked_address == DISPLAY && word(memory + OTHER + 0x80) == DISPLAY);
    } else if (layout == 2 && !retarget) {
        assert(word(memory + LINK + 0x20) == DISPLAY && word(display_area + 0x80) == DISPLAY);
    } else {
        assert(word(memory + OTHER + 0x80) == DISPLAY);
    }
}

int main(void)
{
    static const u32 flags[] = {0, 2, 4, 6, 0x40000, 0x40002, 0x80002, 0x802};
    for (size_t f = 0; f < sizeof flags / sizeof *flags; ++f)
        for (unsigned round = 0; round < 3; ++round)
            for (int layout = 0; layout < 3; ++layout)
                for (int retarget = 0; retarget <= 1; ++retarget)
                    check_case(flags[f], round, layout, retarget);
    return 0;
}
