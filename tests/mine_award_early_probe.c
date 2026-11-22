#include <assert.h>
#include <string.h>

#include "mine_award_early.h"

MineAwardVector D_80126B30;
u16 D_80126B38;
s16 D_80126B3A, D_8011F9C8;

static char events[32];
static unsigned count;
static s32 query_result, movement_result;
static int change_high_bit;
static u8 player[0x217];

static void visit(char service)
{
    assert(count + 1 < sizeof events);
    events[count++] = service;
}

void func_80147060(u8 *actor)
{
    assert(actor == player);
    visit('S');
    actor[0x4d] = 1;
}

void func_80171A1C(u8 *actor)
{
    assert(actor == player);
    visit('A');
    ++actor[0x214];
    actor[0x215] = actor[0x216] = 0;
}

s32 func_800D0EC4(void)
{
    visit('Q');
    if (change_high_bit)
        D_80126B38 ^= 0x8000;
    return query_result;
}

void func_8013E588(void)
{
    visit('H');
}

void func_80174650(u8 *actor)
{
    assert(actor == player && D_8011F9C8 == -1);
    visit('L');
}

void func_80171A34(u8 *actor, s32 phase)
{
    assert(actor == player && phase == 7 && D_8011F9C8 == 2);
    visit('J');
    actor[0x214] = (u8)phase;
}

s32 func_80171CC4(u8 *actor, u8 *vector)
{
    assert(actor == player && vector == D_80126B30.bytes);
    visit('M');
    return movement_result;
}

static void reset(void)
{
    memset(events, 0, sizeof events);
    memset(player, 0, sizeof player);
    count = 0;
    player[0x215] = 0xab;
    player[0x216] = 0xcd;
    D_8011F9C8 = -1;
    change_high_bit = 0;
    query_result = 0;
}

int main(void)
{
    static const s16 selectors[] = {-32768, -1, 0, 1, 110, 111, 32767};
    for (unsigned i = 0; i < sizeof selectors / sizeof *selectors; ++i) {
        for (unsigned mode = 0; mode < 4; ++mode) {
            reset();
            D_80126B3A = selectors[i];
            D_80126B38 = 0x8000;
            query_result = mode & 1;
            change_high_bit = (mode & 2) != 0;
            func_80173B88(player);
            const char *expected = selectors[i] == 0 || selectors[i] >= 111 ? "SAA" :
                query_result ? "SAQA" : change_high_bit ? "SAQLJ" : "SAQHA";
            assert(!strcmp(events, expected));
            assert(player[0x4d] == 1 && player[0x215] == 0 && player[0x216] == 0);
            assert(player[0x214] == (strchr(events, 'J') ? 7 : 2));
        }
    }
    for (unsigned byte = 6; byte <= 7; ++byte) {
        for (int move = 0; move < 3; ++move) {
            reset();
            memset(&D_80126B30, 0, sizeof D_80126B30);
            D_80126B30.bytes[byte] = move ? 0x80 : 0;
            movement_result = move == 2 ? -1 : 0;
            func_80173C64(player);
            assert(!strcmp(events, move == 0 ? "A" : move == 1 ? "M" : "MA"));
            assert(player[0x214] == (move != 1));
        }
    }
    return 0;
}
