#include "forest_animation_commands.h"
#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <string.h>

enum { FIRST = 0x42100, NEXT = 0x63200, MODEL = 0x74300 };
typedef struct {
    ForestPlayerState actor;
    u32 started, flags, suppress, shown;
    unsigned effects;
} Scene;
typedef struct { const u32 *words; size_t length; } Script;
static Scene scene, pending;
static Script first, next;
static jmp_buf refused;
static unsigned frames, reads, effects_seen;
static u32 read_slots[32];

void bfm_forest_animation_enter(void)
{
    if (++frames > 12) longjmp(refused, 1);
}
u32 bfm_forest_animation_word(u32 address, u32 index)
{
    const Script *script = address == FIRST ? &first : address == NEXT ? &next : NULL;
    if (!script || index >= script->length || reads == 32) longjmp(refused, 1);
    read_slots[reads++] = index;
    return script->words[index];
}
void bfm_forest_animation_model_frame(u32 model, u32 address)
{
    assert(model == MODEL);
    pending.shown = address;
}
void func_801553A8(ForestPlayerState *actor)
{
    assert(actor == &pending.actor);
    pending.effects++;
    effects_seen++;
}
/* Other effect services are outside these synthetic scripts. */
#define UNUSED_EFFECT(name) void name(ForestPlayerState *actor) { (void)actor; assert(0); }
UNUSED_EFFECT(func_80155394)
UNUSED_EFFECT(func_801553C0)
UNUSED_EFFECT(func_80155440)
s32 func_80154A74(ForestPlayerState *a, s32 n) { (void)a; (void)n; assert(0); return 0; }
s32 func_80154B4C(ForestPlayerState *a, u32 n) { (void)a; (void)n; assert(0); return 0; }
void bfm_forest_animation_resource(u32 a, u32 b) { (void)a; (void)b; assert(0); }

static void setup(const u32 *program, size_t words)
{
    memset(&scene, 0, sizeof(scene));
    first = (Script){program, words};
    next = (Script){NULL, 0};
    scene.actor.model = MODEL;
    scene.actor.animation = FIRST;
    scene.actor.animation_flags = 1;
    scene.actor.animation_duration = 7;
    scene.flags = 0xfa004000u;
    effects_seen = 0;
}
static int advance(void)
{
    frames = reads = 0;
    pending = scene;
    if (setjmp(refused)) return 0;
    func_8015444C(&pending.actor, &pending.started, &pending.flags, &pending.suppress);
    scene = pending;
    return 1;
}
static void test_effect_and_frame(void)
{
    const u32 script[] = {4, 0xfedc1234u, 19, 0x741811u};
    setup(script, 4);
    scene.actor.animation_step = 255;
    assert(advance());
    assert(reads == 3 && read_slots[0] == 0 && read_slots[1] == 2 && read_slots[2] == 3);
    assert(frames == 3 && scene.effects == 1 && effects_seen == 1);
    assert(scene.actor.animation_cursor == 4 && scene.actor.animation_step == 1);
    assert(scene.actor.animation_frame == 1 && scene.actor.animation_flags == 0x4007);
    assert(scene.started == 1 && scene.shown == 0x741821u);
}
static void test_script_transfer(void)
{
    const u32 jump[] = {5, NEXT, 5};
    const u32 target[] = {1, 4, 0x98765, 23, 13, 6, 9, 25, 0x30201, 0x54321};
    setup(jump, 3);
    next = (Script){target, 10};
    assert(advance());
    assert(scene.actor.animation == NEXT && scene.actor.animation_cursor == 10);
    /* The scanner counted words 0, 3 and 8 before the displayed frame at 9. */
    assert(scene.actor.animation_frame == 4 && scene.actor.animation_step == 1);
    assert(scene.shown == 0x54331 && frames == 3 && scene.effects == 0);
}
static void test_cursor_wrap(void)
{
    const u32 jump[258] = { [255] = 5, [256] = NEXT, [257] = 0x1ff };
    const u32 target[256] = { [0] = 0x32000, [255] = 13 };
    setup(jump, 258);
    next = (Script){target, 256};
    scene.actor.animation_cursor = 255;
    scene.actor.animation_step = 41;
    scene.actor.animation_frame = 77;
    assert(advance());
    assert(reads == 5 && read_slots[1] == 256 && read_slots[2] == 257);
    assert(scene.actor.animation_cursor == 1 && scene.actor.animation_step == 42);
    assert(scene.actor.animation_frame == 1 && scene.shown == 0x32010);
}
static void test_classifier(void)
{
    const u32 script[] = {5, 0x12345, 1, 13, 24, 0x45678, 3, 0x6789a, 25};
    setup(script, 9);
    reads = 0;
    if (setjmp(refused)) { assert(0); return; }
    assert(func_801549F8(&scene.actor) == 0 && reads == 0);
    scene.actor.animation_cursor = 9;
    assert(func_801549F8(&scene.actor) == 3 && reads == 7);
}
static void test_unreachable_cursor(void)
{
    const u32 script[] = {19, 6, 2};
    setup(script, 3);
    unsigned char before[sizeof(scene)];
    memcpy(before, &scene, sizeof(scene));
    /* The seek from word 1 crosses cursor 2. The checked read refuses word 3. */
    assert(!advance());
    assert(effects_seen == 1 && scene.effects == 0);
    assert(!memcmp(&scene, before, sizeof(scene)));
}
int main(void)
{
    test_effect_and_frame();
    test_script_transfer();
    test_cursor_wrap();
    test_classifier();
    test_unreachable_cursor();
    return 0;
}
