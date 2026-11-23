#include "action22_contact.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

_Static_assert(sizeof(FusionWord) == 4 && sizeof(TwinpeakPollHandle) == 4, "guest word");
_Static_assert(offsetof(TwinpeakPollActor, selected_contact) == 0x184, "contact choice");
_Static_assert(offsetof(TwinpeakPollActor, contact_flag_197) == 0x197, "contact flag");
_Static_assert(offsetof(TwinpeakPollActor, handle) == 0x198, "handle");
_Static_assert(offsetof(TwinpeakPollActor, contact) == 0x1a0, "contact byte");

FusionWord D_800AE6B0;
FusionWord D_8018A208[256];
TwinpeakActionCallback D_80189EAC[32];

enum Step { CLEAR, CLEAN, RESET, IDLE, MODE, ADVANCE, EFFECT, OPEN };
static struct {
    enum Step step;
    FusionWord argument;
} events[4];
static size_t called;
static TwinpeakPollActor state;

static void event(enum Step step, FusionWord argument)
{
    assert(called < sizeof(events) / sizeof(events[0]));
    events[called].step = step;
    events[called].argument = argument;
    ++called;
}

static void same_actor(Action21Actor *actor)
{
    assert(actor == &state.base);
}

void func_8016007C(Action21Actor *actor)
{
    same_actor(actor);
    event(CLEAR, 0);
    actor->current_action = 2;  /* The caller must dispatch the current slot. */
}

static void cleanup(Action21Actor *actor)
{
    same_actor(actor);
    event(CLEAN, actor->current_action);
}

void func_80147078(Action21Actor *actor, unsigned int action)
{
    same_actor(actor);
    event(RESET, action);
    actor->current_action = (FusionHalf)action;
    actor->state.phase = 0;
}

void func_80159B70(Action21Actor *actor)
{
    same_actor(actor);
    assert(actor->current_action == 0 && actor->state.phase == 0);
    event(IDLE, 0);
}

void func_8014BFF4(Action21Actor *actor, unsigned int value)
{
    same_actor(actor);
    assert(!state.contact_flag_be && !state.contact_flag_bf && !state.contact_flag_197);
    assert(state.contact == (unsigned char)state.selected_contact);
    event(MODE, value);
}

void func_80146CA0(Action21Actor *actor)
{
    same_actor(actor);
    event(ADVANCE, actor->state.phase);
    ++actor->state.phase;
}

void func_80147324(int effect)
{
    event(EFFECT, (unsigned int)effect);
    state.contact = 254;  /* Select a different resource during this call. */
}

TwinpeakPollHandle twinpeak_contact_handle(FusionWord resource)
{
    event(OPEN, resource);
    return 0xf27c3a40u;
}

static void prepare(unsigned int status, int aborting)
{
    memset(&state, 0x5a, sizeof(state));
    state.base.current_action = 22;
    state.base.state.phase = 1;
    state.selected_contact = 0xba7136e5u;
    state.contact = 9;
    D_800AE6B0 = status;
    called = 0;
    TwinpeakPollActor expected = state;
    func_8015FCC8(&state.base);
    if (aborting) {
        assert(called == 4 && events[0].step == CLEAR && events[1].step == CLEAN);
        assert(events[1].argument == 2 && events[2].step == RESET && events[2].argument == 0);
        assert(events[3].step == IDLE);
        expected.base.current_action = expected.base.state.phase = 0;
    } else {
        assert(called == 2 && events[0].step == MODE && events[0].argument == 1);
        assert(events[1].step == ADVANCE && events[1].argument == 1);
        expected.contact_flag_be = expected.contact_flag_bf = expected.contact_flag_197 = 0;
        expected.contact = 0xe5;
        expected.base.state.phase = 2;
    }
    assert(!memcmp(&state, &expected, sizeof(state)));
}

int main(void)
{
    D_80189EAC[2] = cleanup;
    prepare(0x80, 1);
    prepare(0x80000000u, 1);
    prepare(0x7f000000u, 0);

    D_8018A208[254] = 0xf3914bd3u;
    TwinpeakPollActor expected = state;
    called = 0;
    func_8015FD74(&state.base);
    assert(called == 3 && events[0].step == EFFECT && events[0].argument == 24);
    assert(events[1].step == OPEN && events[1].argument == D_8018A208[254]);
    assert(events[2].step == ADVANCE && events[2].argument == 2);
    expected.handle = 0xf27c3a40u;
    expected.contact = 254;
    expected.base.state.phase = 3;
    assert(!memcmp(&state, &expected, sizeof(state)));
    return 0;
}
