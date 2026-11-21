/* Occupied VBlank slot 7, using the same CPU and scheduler as the other IRQ
 * cases. Require a fresh directory so this probe cannot attach an existing save. */
#include "../pc_port/card/player_card.h"
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

typedef struct CardRequestFixture {
    MusashiPlayerCard player;
    MusashiBiosEvents events;
    unsigned selectors[3], accepted, attempts, polls;
    int reject_file, reject_event;
} CardRequestFixture;

static int request_checkpoint(void *p, const void *cpu) { return p && cpu; }
static void request_console(void *p, const uint8_t *bytes, int32_t count) {
    (void)p; (void)bytes; (void)count;
}
static int request_critical(void *p, int32_t *result) {
    (void)p; *result = 1; return 1;
}
static int request_open(void *p, uint32_t group, uint32_t spec, uint32_t mode,
                        uint32_t callback, int32_t *handle) {
    CardRequestFixture *card = p;
    return musashi_bios_events_open(&card->events, group, spec, mode, callback, handle);
}
static int request_enable(void *p, int32_t handle, int32_t *result) {
    CardRequestFixture *card = p;
    return musashi_bios_events_enable(&card->events, handle, result);
}
static int request_test_event(void *p, int32_t handle, int32_t *result) {
    CardRequestFixture *card = p;
    ++card->polls;
    return !card->reject_event && musashi_bios_events_test(&card->events, handle, result);
}
static int request_vector(void *p, uint32_t vector, uint32_t selector,
                          const uint32_t args[4], uint32_t *result) {
    CardRequestFixture *card = p;
    uint32_t error = 0;
    ++card->attempts;
    if (card->reject_file) return 0;
    assert(vector == 0xa0 && args[0] == 0 && card->accepted < 3);
    if (!musashi_player_card_vector(&card->player, vector, selector, args, result, &error))
        return 0;
    assert(*result == 1);
    card->selectors[card->accepted++] = selector;
    return 1;
}

static void test_card_requests(Machine *m, const char *directory) {
    CardRequestFixture card = {0};
    MusashiBootMemory *ram = &m->memory;
    MusashiPsyCrossIrqScheduler *scheduler = &m->scheduler;
    MusashiEventDevice events = {.userdata=&card, .open_event=request_open,
        .enable_event=request_enable, .b0_0b=request_test_event,
        .enter_critical_section=request_critical, .exit_critical_section=request_critical};
    MusashiCallbackDevice callbacks = {.userdata=&card, .checkpoint=request_checkpoint};
    MusashiCdDevice files = {.userdata=&card, .file_vector=request_vector};
    MusashiStartupPrefixDevice producer = {.callback=&callbacks, .cd=&files, .event=&events};
    MusashiGuestIrqCard devices = {.memory=ram, .files=&files, .events=&events,
        .registered_events=&card.events};
    const int good = !strcmp(m->mode, "request-complete");
    const int other_owner = !strcmp(m->mode, "request-owner");
    const int unknown_handler = !strcmp(m->mode, "request-handler");
    const int wrong_handle = !strcmp(m->mode, "request-handle");
    card.reject_file = !strcmp(m->mode, "request-file");
    const int reject_event = !strcmp(m->mode, "request-event");
    assert(good || other_owner || unknown_handler || wrong_handle || card.reject_file || reject_event);
    uint32_t original[32];
    memcpy(original, m->outer, sizeof(original));
    musashi_bios_events_init(&card.events);
    int32_t result, foreign = -1;
    assert(musashi_boot_call_8006189c(ram, &events, &result));
#ifdef _WIN32
    assert(_mkdir(directory) == 0);
#else
    assert(mkdir(directory, 0700) == 0);
#endif
    assert(musashi_player_card_open(&card.player, ram, &card.events, directory));
    cs_init_card(card.player.service);
    cs_start_card(card.player.service);
    cs_init_files(card.player.service);
    put(ram, 0x8006cbb4u, 0x800616d0u);
    put(ram, 0x80072a34u, UINT32_MAX);
    assert(musashi_irq_controller_write16(&m->irq, 0x1f801074u, 1));
    static MusashiBootMemory other_memory;
    if (other_owner) devices.memory = &other_memory;
    m->card = &devices;

    const uint32_t callers[] = {0x8005fd58u, 0x8005ffb4u};
    const uint32_t handlers[] = {0x8005fdd4u, 0x80060030u};
    card.reject_event = reject_event;
    unsigned pulses = 0;
    for (unsigned command = 0; command < (good ? 2u : 1u); ++command) {
        MusashiResetGraphPrefixStop stop = {0};
        assert(musashi_boot_execute_source_function(ram, &producer, callers[command],
            request_console, NULL, &stop) == MUSASHI_RESETGRAPH_PREFIX_COMPLETE);
        assert(word(ram, 0x80072a34u) == 0 && word(ram, 0x80078d78u) == handlers[command]);
        if (unknown_handler) put(ram, 0x80078d78u, 0x801f0000u);
        if (wrong_handle) {
            const uint32_t group = 0xf7000021u, spec = 0x20u;
            assert(musashi_bios_events_open(&card.events, group, spec, 0x2000u, 0, &foreign));
            assert(musashi_bios_events_enable(&card.events, foreign, &result));
            assert(musashi_bios_events_deliver_poll(&card.events, group, spec));
            put(ram, 0x80078c3cu, (uint32_t)foreign);
        }
        const unsigned epochs = command ? 6u : 2u;
        for (unsigned epoch = 1; epoch <= (good ? epochs : 1u); ++epoch) {
            unsigned previous_calls = m->calls, previous_polls = card.polls;
            uint32_t tick = word(ram, 0x8006cbb8u);
            musashi_player_card_pulse(&card.player, ++pulses);
            assert(musashi_irq_controller_raise_vblank(&m->irq));
            int completed = musashi_psycross_irq_scheduler_dispatch_pending(scheduler, m->outer);
            MusashiCpuStatusSnapshot state;
            assert(musashi_cpu_status_snapshot(scheduler->cpu_status, &state));
            assert(m->calls == previous_calls + 1 && !memcmp(original, m->outer, sizeof(original)));
            if (!good) {
                assert(!completed && state.active && state.faulted && scheduler->faulted);
                assert(!scheduler->return_consumed && !word(ram, 0x80078c88u));
                assert(word(ram, 0x80072a34u) == 0 && cs_idle(card.player.service));
                assert(card.accepted == 0 && card.attempts == (unsigned)card.reject_file);
                if (!card.reject_file) assert(card.polls == previous_polls + (unsigned)card.reject_event);
                if (wrong_handle) {
                    assert(musashi_bios_events_test(&card.events, foreign, &result) && result == 1);
                    assert(musashi_bios_events_test(&card.events, foreign, &result) && !result);
                }
                assert(!musashi_psycross_irq_scheduler_dispatch_pending(scheduler, m->outer));
                assert(m->calls == previous_calls + 1);
                break;
            }
            assert(completed && !state.active && !state.faulted && state.sequence == pulses);
            assert(scheduler->last_result == MUSASHI_IRQ_CONTEXT_RETURN_DELEGATED && m->stop.pc == 0xb0u);
            assert(!m->irq.status && word(ram, 0x8006cbb8u) == tick + 1);
            assert((word(ram, 0x80072a34u) == UINT32_MAX) == (epoch == epochs));
            assert(word(ram, 0x80078c88u) == (unsigned)(epoch == epochs));
        }
        if (good) {
            assert(cs_idle(card.player.service));
            assert(word(ram, 0x80078cc4u) == command + 1u);
            assert(word(ram, 0x80078cc8u) == (command ? 0u : 3u));
            assert(card.accepted == (command ? 3u : 1u));
        }
    }
    if (good) assert(card.selectors[0] == 0xab && card.selectors[1] == 0xab && card.selectors[2] == 0xac);
    m->card = NULL;
    musashi_player_card_close(&card.player);
}
