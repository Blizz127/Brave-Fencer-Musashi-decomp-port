#include "action22_start.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

_Static_assert(offsetof(TwinpeakPollActor, selected_contact) == 0x184, "word offset");
_Static_assert(offsetof(Action21Actor, current_action) == 0, "action selector offset");

enum Event { QUERY, CLEAR, CALLBACK, SET, AFTER_3, AFTER_4, CONTINUE };
static struct {
    FusionWord answer;
    unsigned int selected;
    TwinpeakPollActor actor;
    enum Event events[6];
    unsigned int count;
} fixture;

static void record(Action21Actor *actor, enum Event e)
{
    assert(actor == &fixture.actor.base);
    assert(fixture.count < sizeof(fixture.events) / sizeof(fixture.events[0]));
    fixture.events[fixture.count++] = e;
}
FusionWord func_801612B8(Action21Actor *actor, unsigned int argument)
{
    record(actor, QUERY);
    assert(argument == 0);
    return fixture.answer;
}
void func_8016007C(Action21Actor *actor)
{
    record(actor, CLEAR);
    /* A real boundary can mutate the actor; the table selector is read afterwards. */
    actor->current_action = 2;
    fixture.actor.selected_contact = 0x12345678u;
}
static void cleanup(Action21Actor *actor)
{
    record(actor, CALLBACK);
    assert(fixture.actor.selected_contact == 0x92345678u);
    assert(actor->current_action == 2);
}
TwinpeakActionCallback D_80189EAC[3] = { 0, 0, cleanup };
void func_80147078(Action21Actor *actor, unsigned int selection)
{
    record(actor, SET);
    fixture.selected = selection;
}
void func_8015BE94(Action21Actor *actor) { record(actor, AFTER_3); }
void func_8015C0C4(Action21Actor *actor) { record(actor, AFTER_4); }
void func_8015FBE0(Action21Actor *actor) { record(actor, CONTINUE); }

static void run(FusionWord answer, unsigned int selected)
{
    static const enum Event otherwise[] = { QUERY, CONTINUE };
    enum Event replacement[] = { QUERY, CLEAR, CALLBACK, SET, selected == 3 ? AFTER_3 : AFTER_4 };
    memset(&fixture, 0, sizeof(fixture));
    fixture.actor.base.current_action = 1;
    fixture.actor.selected_contact = 0xabcdu;
    fixture.answer = answer;
    func_8015FAE8(&fixture.actor.base);
    assert(fixture.selected == selected);
    if (selected) {
        assert(fixture.count == sizeof(replacement)/sizeof(replacement[0]));
        assert(memcmp(fixture.events, replacement, sizeof(replacement)) == 0);
    } else {
        assert(fixture.count == sizeof(otherwise)/sizeof(otherwise[0]));
        assert(memcmp(fixture.events, otherwise, sizeof(otherwise)) == 0);
        assert(fixture.actor.base.current_action == 1 && fixture.actor.selected_contact == 0xabcdu);
    }
}
int main(void)
{
    run(0, 3);
    run(0x8000, 3);
    run(0x4000, 4);
    run(0xc000, 4);
    run(0xffffffffu, 4);
    run(0x80004000u, 4);
    run(1, 0);
    run(0x8001, 0);
    run(0x18000, 0);
    run(0x80000000u, 0);
    return 0;
}
