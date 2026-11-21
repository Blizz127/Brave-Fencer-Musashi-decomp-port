#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "../src/overlays/sc01_0000/801473dc.c"
#include "../src/overlays/sc01_0000/801473ec.c"
#include "../src/overlays/sc01_0000/80147814.c"
#include "../src/overlays/sc01_0000/80147948.c"
#include "../src/overlays/sc01_0000/801479bc.c"
#include "../src/overlays/sc01_0000/80147b5c.c"
#include "../src/overlays/sc01_0000/80148038.c"
#include "../src/overlays/sc01_0000/801488a8.c"
#include "../src/overlays/sc01_0000/8014891c.c"
#include "../src/overlays/sc01_0000/80148980.c"
#include "../src/overlays/sc01_0000/80148a48.c"
#include "../src/overlays/sc01_0000/80149744.c"
#include "../src/overlays/sc01_0000/80149884.c"
#include "../src/overlays/sc01_0000/80149cd4.c"
#include "../src/overlays/sc01_0000/80149fb0.c"
#include "../src/overlays/sc01_0000/8016f1ac.c"

s32 D_80127090, D_80127094, D_80127098;
u8 D_80078EBF, D_80078E78[0x50];
s16 D_800B9A08;
s32 D_801807C0[] = { 22, -3, 0, 91 };
u32 D_80126D50;

typedef union {
    u8 bytes[0x200];
    u32 words[0x80];
    Obj801473dc movement;
    Obj80149744 action;
} Actor;

static void put16(Actor *actor, unsigned offset, u16 value) {
    memcpy(actor->bytes + offset, &value, sizeof(value));
}

static void integrate_selected_axes(void) {
    unsigned mask;
    s32 supplied[] = {-5, 3, -1};
    for (mask = 0; mask < 8; ++mask) {
        Actor actor = {0};
        u32 expected[3];
        actor.words[0x44 / 4] = (mask << 22) | 4;
        actor.words[0x24 / 4] = 9;
        actor.words[0x28 / 4] = (u32)-7;
        actor.words[0x2c / 4] = 0;
        func_80147948(actor.bytes, 9, 10);
        func_801479BC((char *)actor.bytes, -9, 10);
        assert(actor.words[0x24 / 4] == 10 && actor.words[0x28 / 4] == (u32)-10);
        actor.words[1] = UINT32_MAX;
        actor.words[2] = 0x80000000u;
        actor.words[3] = 0;
        for (unsigned i = 0; i < 3; ++i)
            expected[i] = mask & (1u << i) ? actor.words[0x24 / 4 + i] : (u32)supplied[i];
        func_80147B5C(actor.bytes, supplied);
        assert(!D_80127090 && !D_80127094 && !D_80127098);
        for (unsigned i = 0; i < 3; ++i) assert(actor.words[4 + i] == expected[i]);
        func_801473EC(actor.bytes);
        assert(actor.words[1] == UINT32_MAX + expected[0]);
        assert(actor.words[2] == 0x80000000u + expected[1]);
        assert(actor.words[3] == expected[2]);
        assert(actor.words[0x44 / 4] == ((mask << 22) | 0x80000004u));
        func_801473DC(&actor.movement);
        assert(!(actor.words[4] | actor.words[5] | actor.words[6]));
        actor.words[0x44 / 4] = 4;
        func_801473EC(actor.bytes);
        assert(actor.words[0x44 / 4] == 4 && actor.words[1] == UINT32_MAX + expected[0]);
    }
}

static void check_limits(void) {
    Actor actor = {0};
    actor.words[0x24 / 4] = INT32_MAX;
    func_80147948(actor.bytes, 1, 17);
    assert(actor.words[0x24 / 4] == 0x80000000u);
    actor.words[0x28 / 4] = 0x80000000u;
    func_801479BC((char *)actor.bytes, -1, 17);
    assert(actor.words[0x28 / 4] == INT32_MAX);
    for (int sign = -1; sign <= 1; sign += 2) {
        const s32 bound = sign * 0x1f0000;
        actor.words[0x34 / 4] = (u32)(bound - sign);
        func_80147814(actor.bytes, sign * 5);
        assert(actor.words[0x34 / 4] == (u32)bound);
        actor.words[0x34 / 4] = (u32)(bound - sign);
        func_80148038(actor.bytes, sign * 5);
        assert(actor.words[0x34 / 4] == (u32)(bound + sign * 4));
        func_80148038(actor.bytes, sign * 8);
        assert(actor.words[0x34 / 4] == (u32)(bound + sign * 4));
        func_80148038(actor.bytes, -sign * 5);
        assert(actor.words[0x34 / 4] == (u32)(bound - sign));
    }
}

static void check_controller(void) {
    Actor actor = {0};
    const u8 types[] = {0x41, 0x53, 0x73, 0xff};
    for (unsigned mode = 0; mode < sizeof(types); ++mode) {
        actor.bytes[0xa9] = types[mode];
        for (unsigned dpad = 0; dpad < 16; ++dpad) {
            unsigned buttons = (dpad << 12) | 0x123;
            put16(&actor, 0xaa, (u16)buttons);
            for (unsigned y = 0; y < 256; ++y) {
                const unsigned x = (y * 37u) & 255;
                put16(&actor, 0xae, (u16)((y << 8) | x));
                const int digital = mode == 0, analog = mode == 1 || mode == 2;
                assert(func_801488A8(actor.bytes) == (digital ? (int)(buttons & 0xf000) : analog && (x != 128 || y != 128)));
                assert(func_8014891C(actor.bytes) == (digital ? (int)(buttons & 0x1000) : analog && y < 64));
                assert(func_80148980(actor.bytes) == (digital ? (int)(buttons & 0x4000) : analog && y > 192));
                assert(func_80148A48(actor.bytes) == (digital ? (int)(buttons & 0x2000) : analog && x > 192));
            }
        }
    }
}

static void check_actor_input_gates(void) {
    Actor actor = {0};
    D_80126D50 = 0x7f000000;
    D_800B9A08 = 91;
    assert(func_8016F1AC() == 0 && func_80149884() == 1);
    D_80126D50 |= 0x80000001u;
    assert(func_8016F1AC() == 0x80000001u && func_80149884() == 0);
    D_800B9A08 = 0x308f;
    D_80126D50 = 0;
    assert(func_80149884() == 0);

    put16(&actor, 0xaa, 8);
    put16(&actor, 0xac, 0x180);
    D_80078EBF = 0x7f;
    assert(!func_80149744(&actor.action) && func_80149CD4(actor.bytes));
    D_80078EBF = 0x80;
    assert(func_80149744(&actor.action));
    actor.words[0x44 / 4] = 4;
    assert(!func_80149744(&actor.action) && !func_80149CD4(actor.bytes));
    actor.words[0x44 / 4] = 0;
    D_80078E78[0x48] = 0x23;
    D_800B9A08 = -3;
    assert(!func_80149FB0(actor.bytes) && D_80078E78[0x48] == 0x23);
    D_800B9A08 = 91; /* The table's zero terminates before its final element. */
    assert(func_80149FB0(actor.bytes) && D_80078E78[0x48] == 0xa3);
    assert(func_80149FB0(actor.bytes) && D_80078E78[0x48] == 0x23);
    put16(&actor, 0x1c8, 0xffff);
    assert(!func_80149FB0(actor.bytes) && !func_80149CD4(actor.bytes));
    put16(&actor, 0x1c8, 0);
    actor.words[0x44 / 4] = 0x400;
    assert(!func_80149FB0(actor.bytes) && !func_80149CD4(actor.bytes));
    actor.words[0x44 / 4] = 0;
    put16(&actor, 0xaa, 0);
    put16(&actor, 0xac, 0);
    assert(!func_80149FB0(actor.bytes) && !func_80149CD4(actor.bytes));
    assert(D_80078E78[0x48] == 0x23);
}

int main(void) {
    integrate_selected_axes();
    check_limits();
    check_controller();
    check_actor_input_gates();
    return 0;
}
