#include "action22_poll.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

_Static_assert(offsetof(Action21Actor, current_action) == 0, "action");
_Static_assert(offsetof(Action21Actor, state.phase) == 2, "phase");
_Static_assert(offsetof(Action21Actor, reserved) == 2, "action 21 byte view");
_Static_assert(sizeof(((Action21Actor *)0)->reserved) == 0xb6, "action 21 byte count");
_Static_assert(offsetof(Action21Actor, frame_events) == 0xb8, "events");
_Static_assert(offsetof(Action21Actor, next_frame_events) == 0xba, "queued events");
_Static_assert(sizeof(Action21Actor) == 0xbc, "observed actor prefix");

_Static_assert(offsetof(TwinpeakPollActor, handle) == 0x198, "guest handle");
_Static_assert(sizeof(TwinpeakPollHandle) == 4, "32-bit guest handle");
_Static_assert(offsetof(TwinpeakPollActor, contact) == 0x1a0, "contact selector");

static TwinpeakPollActor poll_actor;
#define subject poll_actor.base
static int ready, disabled;
static unsigned int contact_result;
static char calls[12];
static unsigned count;

static void record(Action21Actor *actor, char event)
{
    assert(actor == &subject && count + 1 < sizeof calls);
    calls[count++] = event;
    calls[count] = '\0';
}

static void expect(const char *sequence)
{
    assert(strcmp(calls, sequence) == 0);
    count = 0;
    calls[0] = '\0';
}

static void phase_zero(Action21Actor *actor)
{
    record(actor, 'P');
    assert(actor->current_action == 22 && actor->state.phase == 0);
}

static void cleanup(Action21Actor *actor)
{
    record(actor, 'C');
    assert(actor->current_action == 21 || actor->current_action == 22);
    actor->reserved[5] = 0x5a;
}

TwinpeakActionCallback D_80189EAC[] = { [21] = cleanup, [22] = cleanup };
TwinpeakActionCallback D_8018A1F4[] = { [0] = phase_zero, [3] = func_8015FDCC, [4] = func_8015FE70 };
FusionQueueSlot D_8011F9D0[8];

FusionWord bfm_twinpeak_actor_word(const Action21Actor *actor)
{
    assert(actor == &subject);
    return 17;
}

FusionWord func_80146750(FusionRequest *request)
{
    (void)request;
    assert(0);
    return 0;
}

void func_80147078(Action21Actor *actor, unsigned int action)
{
    assert(action == 0 || action == 22);
    record(actor, action ? 'B' : 'Z');
    actor->current_action = action;
    actor->state.phase = 0;
}

void func_80154A74(Action21Actor *actor, unsigned int index)
{
    assert(index == 17 && (actor->frame_events & 0x4000));
    record(actor, 'A');
}

void func_80159B70(Action21Actor *actor)
{
    assert(actor->current_action == 0 && actor->reserved[5] == 0x5a);
    record(actor, 'I');
}

void func_80161208(Action21Actor *actor)
{
    record(actor, 'T');
}

int func_801399F0(TwinpeakPollHandle handle)
{
    assert(handle == poll_actor.handle);
    record(&subject, 'P');
    return ready;
}

void func_80147324(int effect)
{
    if (effect == 25) {
        record(&subject, 'B');
        poll_actor.handle = 29;
    } else {
        assert(effect == 2441 && D_8011F9D0[0].kind == 36);
        record(&subject, 'E');
    }
}

void func_80139914(TwinpeakPollHandle handle)
{
    assert(handle == 29);
    record(&subject, 'C');
    poll_actor.handle = 31;
}

int func_80028DE0(void)
{
    assert(poll_actor.handle == 0);
    record(&subject, 'D');
    poll_actor.contact = 177;
    return disabled;
}

unsigned int func_80165700(Action21Actor *actor, unsigned int contact)
{
    assert(contact == 177);
    record(actor, 'H');
    return contact_result;
}

void func_80146CA0(Action21Actor *actor)
{
    record(actor, 'N');
    ++actor->state.phase;
}

static void check_poll(void)
{
    subject.state.phase = 3;
    poll_actor.handle = 19;
    func_8015FAAC(&subject);
    expect("P");
    assert(subject.state.phase == 3 && poll_actor.handle == 19);

    ready = -1;
    disabled = 1;
    func_8015FAAC(&subject);
    expect("PBCDN");
    assert(subject.state.phase == 4);

    subject.state.phase = 3;
    disabled = 0;
    contact_result = 256;
    func_8015FAAC(&subject);
    expect("PBCDHN");
    assert(subject.state.phase == 4 && D_8011F9D0[0].kind == 0);

    subject.state.phase = 3;
    contact_result = 257;
    func_8015FAAC(&subject);
    expect("PBCDHEN");
    assert(subject.state.phase == 4 && D_8011F9D0[0].actor == 17);
}

int main(void)
{
    subject.current_action = 21;
    subject.frame_events = 0x8000;
    func_8015F89C(&subject);
    expect("CZI");

    subject.state.phase = 4;
    func_8015FA78(&subject);
    expect("BP");

    subject.state.phase = 4;
    subject.next_frame_events = 0;
    func_8015FAAC(&subject);
    expect("CZI");

    subject.current_action = 22;
    subject.frame_events = 0x8000;
    subject.next_frame_events = 0x4001;
    func_8015FE70(&subject);
    expect("AT");
    assert(subject.frame_events == 0x4001);

    subject.next_frame_events = 0;
    func_8015FE70(&subject);
    expect("AT");

    subject.frame_events = 0x8000;
    subject.next_frame_events = 1;
    func_8015FE70(&subject);
    expect("T");
    assert(subject.frame_events == 1);
    check_poll();
    return 0;
}
