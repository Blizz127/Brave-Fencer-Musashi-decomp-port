#include "room_event.h"
#include <assert.h>
#include <stdint.h>
#include <setjmp.h>
#include <stddef.h>
#include <string.h>

/* Controlled service fixtures. Each event has distinct bytes on both sides. */
typedef struct {
    u8 before[3];
    Bfm304cEvent event;
    u8 after[14];
} Record;

_Static_assert(offsetof(Bfm304cEvent, phase) == 0x15, "event phase offset");
_Static_assert(sizeof(Bfm304cEvent) == 22, "room event prefix");
_Alignas(8) static Record records[4], wanted[4];
Bfm304cEventStage D_801862EC[2];
static Bfm304cEventStage wanted_table[2];
static s32 unexpected_handler(Bfm304cEvent *event);
static jmp_buf stopped;
static s32 result;

static void fill_records(unsigned seed)
{
    unsigned char *bytes = (unsigned char *)records;
    for (size_t i = 0; i < sizeof records; ++i)
        bytes[i] = (u8)(i * 13u + seed * 31u);
    memcpy(wanted, records, sizeof records);
}

static void compare_records(void)
{
    assert(memcmp(records, wanted, sizeof records) == 0);
    assert(D_801862EC[0] == wanted_table[0] && D_801862EC[1] == wanted_table[1]);
}

static void set_table(Bfm304cEventStage first, Bfm304cEventStage second)
{
    D_801862EC[0] = wanted_table[0] = first;
    D_801862EC[1] = wanted_table[1] = second;
}

static void phase_at(unsigned slot, u8 value)
{
    records[slot].event.phase = value;
    wanted[slot].event.phase = value;
}

static void mark(unsigned slot, unsigned offset, u8 value)
{
    records[slot].event.prefix[offset] = value;
    wanted[slot].event.prefix[offset] = value;
}

enum Service { RUN_THROUGH, FIRST_SERVICE, FADE };
static enum Service stop_service;
static unsigned active_slot, changes, services;
static u8 first_write, fade_write;

void func_80180C1C(void)
{
    assert(services++ == 0);
    compare_records();
    if (stop_service == FIRST_SERVICE) longjmp(stopped, 1);
    if (changes & 1u) {
        phase_at(active_slot, first_write);
        set_table(unexpected_handler, func_801809D0);
    }
    mark(active_slot, 2, 0xcd);
    phase_at((active_slot + 1) % 4, 0x8d);
}

void func_800167B8(s32 mode)
{
    assert(mode == 0 && services++ == 1);
    compare_records();
    if (stop_service == FADE) longjmp(stopped, 1);
    if (changes & 2u) {
        phase_at(active_slot, fade_write);
        set_table(unexpected_handler, unexpected_handler);
    }
    mark(active_slot, 6, 0x72);
    phase_at((active_slot + 2) % 4, 0xaf);
}

static s32 unexpected_handler(Bfm304cEvent *event)
{
    (void)event;
    assert(0 && "unexpected table entry");
    return 0;
}

static void stage_zero(u8 initial, unsigned edits, enum Service cut, int via_selector)
{
    fill_records(initial + edits);
    active_slot = (initial + edits) % 4;
    phase_at(active_slot, initial);
    services = 0;
    stop_service = cut;
    changes = edits;
    first_write = (u8)(initial ^ 0xa6u);
    fade_write = (u8)(initial ^ 0x59u);
    set_table(func_801809D0, unexpected_handler);
    result = INT32_MIN;

    if (setjmp(stopped) == 0) {
        assert(!via_selector || initial == 0);
        result = via_selector ? func_80180994(&records[active_slot].event) :
                                func_801809D0(&records[active_slot].event);
        assert(cut == RUN_THROUGH && result == 0 && services == 2);
        ++wanted[active_slot].event.phase;
    } else {
        assert(cut != RUN_THROUGH && result == INT32_MIN);
        assert(services == (unsigned)cut);
    }
    compare_records();
}

typedef struct {
    unsigned slot, handler;
    s32 word;
} Visit;

static Visit visits[2];
static unsigned planned, entered, stop_visit;
static unsigned second_map;
static const unsigned maps[4][2] = {{0, 1}, {1, 0}, {0, 0}, {1, 1}};
static void install(unsigned map);

static s32 synthetic_handler(unsigned identity, Bfm304cEvent *event)
{
    assert(entered < planned);
    const unsigned i = entered++;
    const Visit visit = visits[i];
    assert(visit.handler == identity && event == &records[visit.slot].event);
    compare_records();
    if (entered == stop_visit) longjmp(stopped, 1);
    phase_at(visit.slot, (u8)(event->phase ^ 1u));
    mark(visit.slot, 12 + i, (u8)(0x53u + identity + i));
    if (i == 0 && planned == 2) {
        install(second_map);
        s32 inner = func_80180994(&records[visits[1].slot].event);
        assert(inner == visits[1].word);
    }
    return visit.word;
}

static s32 handler_a(Bfm304cEvent *event) { return synthetic_handler(0, event); }
static s32 handler_b(Bfm304cEvent *event) { return synthetic_handler(1, event); }

static void install(unsigned map)
{
    Bfm304cEventStage handlers[] = {handler_a, handler_b};
    set_table(handlers[maps[map][0]], handlers[maps[map][1]]);
}

static void dispatch(unsigned phase, unsigned map, s32 word, unsigned nesting,
                     unsigned cut)
{
    fill_records((unsigned)word + phase + nesting);
    const unsigned slot = (phase + map + nesting) % 4;
    const unsigned peer = nesting == 1 ? slot : (slot + 1) % 4;
    phase_at(peer, (u8)(phase ^ 1u));
    phase_at(slot, (u8)phase);
    entered = 0;
    planned = nesting == 0 ? 1 : 2;
    stop_visit = cut;
    second_map = (map + 1) % 4;
    visits[0] = (Visit){slot, maps[map][phase], word};
    visits[1] = (Visit){peer, maps[second_map][phase ^ 1u], word == 17 ? -1 : 17};
    result = 41;
    install(map);
    if (setjmp(stopped) == 0) {
        result = func_80180994(&records[slot].event);
        assert(cut == 0 && entered == planned && result == word);
    } else {
        assert(cut != 0 && entered == cut && result == 41);
    }
    compare_records();
}

int main(void)
{
    const s32 words[] = {0, 1, 17, 0xff, 0x100, 0xffff, 0x10000,
                         0x70193ca4, INT32_MAX, INT32_MIN, -65536, -1};
    for (unsigned phase = 0; phase <= 255; ++phase)
        for (unsigned edits = 0; edits < 4; ++edits)
            for (enum Service s = RUN_THROUGH; s <= FADE; ++s) {
                stage_zero((u8)phase, edits, s, 0);
                if (phase == 0) stage_zero(0, edits, s, 1);
            }

    for (unsigned phase = 0; phase < 2; ++phase)
        for (unsigned map = 0; map < 4; ++map)
            for (size_t n = 0; n < sizeof words / sizeof words[0]; ++n)
                for (unsigned nest = 0; nest < 3; ++nest)
                    for (unsigned cut = 0; cut < (nest ? 3u : 2u); ++cut)
                        dispatch(phase, map, words[n], nest, cut);
    return 0;
}
