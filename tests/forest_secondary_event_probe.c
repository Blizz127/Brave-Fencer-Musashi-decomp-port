#include "forest_secondary_event.h"
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

ForestPlayerState D_80126B58;
ForestEventSlot D_8011F9D0[20];
s16 D_800B9A08;
s32 D_8012707C;

static ForestPlayerState other;
static u32 address;
static const char *calls;
static char unavailable;
static jmp_buf boundary;
static const u16 position_words[] = {173, 0xfb02, 2149};

static void called(char id)
{
    assert(*calls == id);
    ++calls;
    if (unavailable == id)
        longjmp(boundary, 1);
}

void bfm_forest_secondary_sound(u16 command, u16 argument)
{
    assert(command == 4 && argument == 0x8e0);
    called('s');
}

void func_8012E5CC(void *position, u16 command, u16 argument)
{
    assert(command == 4 && argument == 0x8e0);
    assert(memcmp(position, position_words, sizeof(position_words)) == 0);
    called('p');
}

void bfm_forest_secondary_command(u16 command, u16 argument)
{
    assert(command == 0x8e0 && argument == 0);
    called('c');
}

void bfm_forest_secondary_update(ForestPlayerState *actor)
{
    assert(actor == &other);
    called('u');
}

u32 bfm_forest_action_actor_address(ForestPlayerState *actor)
{
    assert(actor == &other);
    called('a');
    return address;
}

static int invoke(int *value)
{
    if (setjmp(boundary))
        return 0;
    *value = func_80149954(&other);
    return 1;
}

static void run(u32 flags, u16 aa, u16 ac, s16 room, s32 hint,
                int free_slot, const char *sequence, char missing, int accepts)
{
    ForestEventSlot expected[20];
    memset(D_8011F9D0, 0x75, sizeof(D_8011F9D0));
    for (int i = 0; i < 20; ++i)
        D_8011F9D0[i].kind = (i == free_slot || i < 8) ? 0 : (u16)(10 + i);
    memcpy(expected, D_8011F9D0, sizeof(expected));

    other.flags = flags;
    other.field_aa = aa;
    other.field_ac = ac;
    D_800B9A08 = room;
    D_8012707C = hint;
    calls = sequence;
    unavailable = missing;

    int value;
    if (invoke(&value)) {
        assert(value == accepts);
        assert(!missing);
        if (accepts && free_slot >= 8) {
            ForestEventSlot *slot = expected + free_slot;
            slot->kind = 0x12;
            slot->x = slot->z = 0;
            slot->y = -30;
            slot->actor = address;
            slot->first = slot->second = 0;
        }
    } else {
        assert(missing);
        /* An unresolved provider returns control to a transaction for the actor. */
        if (aa && (ac & 0x90) && hint == 0x8e0)
            assert(D_8012707C == 0);
        D_8012707C = hint;
    }
    assert(*calls == '\0');
    assert(memcmp(expected, D_8011F9D0, sizeof(expected)) == 0);
    assert(D_8012707C == (accepts && !missing && (ac & 0x90) && hint == 0x8e0
                           ? 0 : hint));
}

int main(void)
{
    for (int i = 0; i < 3; ++i)
        memcpy((unsigned char *)&D_80126B58 + 6 + 4 * i, position_words + i, 2);
    memset(&other, 0x87, sizeof(other));
    address = 0x80220780u;
    if (address == (u32)(uintptr_t)&other)
        address += 4;

    run(0x404, 0x90, 0x90, 0, 0x8e0, 8, "", 0, 0);
    run(0, 0, 0, 0, 77, 10, "s", 0, 0);
    run(0, 0, 0, 0x308d, 77, 10, "p", 0, 0);
    run(0, 0, 0, 0, 77, 10, "s", 's', 0);
    run(0, 0, 0, 0x308d, 77, 10, "p", 'p', 0);
    run(0, 0x80, 0, 0, 77, 9, "ua", 0, 1);
    run(0, 0x10, 0x10, 0, 77, 19, "cua", 0, 1);
    run(0, 0x10, 0x10, 0, 0x8e0, -1, "ua", 0, 1);
    run(0, 0x10, 0x10, 0, 77, 8, "c", 'c', 0);
    run(0, 0x10, 0x10, 0, 0x8e0, 8, "u", 'u', 0);
    run(0, 0x80, 0x80, 0, 0x8e0, 8, "ua", 'a', 0);
    return 0;
}
