/* No disc, native UI or filesystem backend. Both clocks use invented counters. */
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "musashi_device_epoch.h"
#include "player_card.h"

typedef struct NativeBoot {
    MusashiDeviceEpoch epoch;
    MusashiPlayerCard file_card;
    struct { MusashiPsyCrossIrqScheduler scheduler; } irq;
    int guest_clock;
    uint64_t guest_tick, guest_sequence, guest_next_vblank;
} NativeBoot;

static NativeBoot boot;
static MusashiIrqSourceBatch host_cut;
static uint64_t owner_id = 1, host_stop_signal;
static uint64_t reject_edge, stop_at_edge, stop_at_pulse;
static uint64_t pulses[12], edges[12];
static unsigned pulse_count, edge_count, collections, snapshots, attempts, accepted;
static int refuse_event, early_wait;
static uint8_t destination[CS_SECTOR_BYTES];

static uint64_t SDL_GetPerformanceFrequency(void) { return MUSASHI_DEVICE_CLOCK_HZ; }
static uint64_t SDL_GetPerformanceCounter(void) { return host_cut.cut_tick; }

int musashi_psycross_irq_scheduler_collect_source(MusashiPsyCrossIrqScheduler *scheduler,
                                                   MusashiIrqSourceBatch *batch) {
    assert(scheduler == &boot.irq.scheduler);
    ++collections;
    *batch = host_cut;
    host_cut.count = 0;
    return 1;
}

#include "providers.inc"
#include "pulse.inc"

static void observed_pulse(MusashiPlayerCard *card, uint64_t sequence) {
    assert(card == &boot.file_card && owner_id == boot.epoch.owner && !boot.epoch.busy);
    assert(sequence <= boot.epoch.last_sequence && pulse_count < 12);
    pulses[pulse_count++] = sequence;
    musashi_player_card_pulse(card, sequence);
    if (sequence == stop_at_pulse) host_stop_signal = 1;
}

/* Keep every actual wrapper invocation observable, including a stale idle pulse
 * that the player card itself could otherwise suppress. */
#define musashi_player_card_pulse observed_pulse
#include "advance.inc"
#undef musashi_player_card_pulse

static uint64_t owner(void *context) { assert(context == &boot); return owner_id; }

static int wait_until(void *context, uint64_t target) {
    assert(context == &boot && boot.epoch.busy);
    if (!early_wait) {
        if (boot.guest_clock) boot.guest_tick = target;
        else host_cut.cut_tick = target;
    }
    return 1;
}

static int advance_devices(void *context, uint64_t cycle) {
    assert(context == &boot && boot.epoch.busy && cycle >= boot.epoch.cycle);
    return 1;
}

static int commit_edge(void *context, uint64_t sequence, uint64_t cycle) {
    assert(context == &boot && boot.epoch.busy && cycle == boot.epoch.cycle);
    if (sequence == reject_edge) return 0;
    assert(edge_count < 12);
    edges[edge_count++] = sequence;
    if (sequence == stop_at_edge) host_stop_signal = 1;
    return 1;
}

static uint8_t *map_buffer(void *context, uint32_t address, size_t size, int writable) {
    assert(context == &boot && address == 1 && size == sizeof(destination) && writable);
    return destination;
}

static CsMedia snapshot(void *context, unsigned slot, uint8_t *image) {
    assert(context == &boot && !slot && !boot.epoch.busy);
    ++snapshots;
    memset(image, 0x5a, CS_IMAGE_BYTES);
    return CS_MEDIA_OK;
}

static int unexpected_write(void *context, unsigned slot, const uint8_t *previous,
                            const uint8_t *replacement) {
    (void)context; (void)slot; (void)previous; (void)replacement;
    assert(0);
    return 0;
}

static int deliver(void *context, uint32_t group, uint32_t spec) {
    assert(context == &boot && !boot.epoch.busy);
    assert(group == CS_EVENT_HW && spec == CS_DONE);
    ++attempts;
    if (refuse_event) return 0;
    ++accepted;
    return 1;
}

static void setup(int guest) {
    boot.guest_clock = guest;
    boot.guest_next_vblank = MUSASHI_GUEST_FRAME_CYCLES;
    boot.irq.scheduler.installed = !guest;
    host_cut.frequency = MUSASHI_DEVICE_CLOCK_HZ;
    MusashiDeviceEpochAdapter adapter = {
        .userdata = &boot, .owner_thread = owner, .collect_source = epoch_collect,
        .wait_until_counter = wait_until, .advance_devices = advance_devices,
        .vblank_edge = commit_edge,
    };
    assert(musashi_device_epoch_init(&boot.epoch, &adapter));
    boot.irq.scheduler.installed = 1;
    CsHost host = {.context = &boot, .map = map_buffer, .snapshot = snapshot,
                   .commit = unexpected_write, .event = deliver};
    boot.file_card.service = cs_create(host);
    assert(boot.file_card.service);
    cs_init_card(boot.file_card.service);
    cs_start_card(boot.file_card.service);
}

static void publish(uint64_t sequence, uint64_t tick) {
    assert(host_cut.count < MUSASHI_IRQ_SOURCE_CAPACITY);
    host_cut.edges[host_cut.count++] = (MusashiIrqSourceEdge){sequence, tick};
    host_cut.cut_tick = tick;
    boot.irq.scheduler.source_sequence = sequence;
}

static void queue_read(void) {
    memset(destination, 0, sizeof destination);
    assert(cs_sector_read(boot.file_card.service, 0, 1, 1));
    assert(!cs_idle(boot.file_card.service));
}

static void selected_guest(void) {
    setup(1);
    queue_read();
    publish(1, 3); /* Host wakes and is drained. Guest clock owns the edges. */
    assert(advance_epoch(&boot, 0));
    assert(collections == 1 && !host_cut.count && !edge_count && !pulse_count);
    assert(!snapshots && !attempts && !destination[0]);
    assert(advance_epoch(&boot, 3u * MUSASHI_GUEST_FRAME_CYCLES));
    assert(boot.epoch.waits == 1 && edge_count == 3 && pulse_count == 3);
    for (unsigned i = 0; i < 3; ++i) assert(edges[i] == i+1 && pulses[i] == i+1);
    assert(snapshots == 1 && accepted == 1 && destination[0] == 0x5a);
}

static void selected_host(void) {
    setup(0);
    boot.guest_tick = 9u * MUSASHI_GUEST_FRAME_CYCLES;
    publish(1, 2); publish(2, 4); publish(3, 7);
    assert(advance_epoch(&boot, 0) && pulse_count == 3 && edge_count == 3);
    for (unsigned i = 0; i < 3; ++i) assert(edges[i] == i+1 && pulses[i] == i+1);
    queue_read(); /* A fresh request after already delivered edges must wait. */
    assert(advance_epoch(&boot, 0) && advance_epoch(&boot, 10));
    assert(pulse_count == 3 && !snapshots && !attempts && !destination[0]);
    refuse_event = 1;
    publish(4, 20);
    assert(advance_epoch(&boot, 0));
    assert(pulses[3] == 4 && snapshots == 1 && attempts == 1 && !accepted);
    assert(!cs_idle(boot.file_card.service) && destination[0] == 0x5a);
    refuse_event = 0;
    assert(advance_epoch(&boot, 0) && attempts == 1 && pulse_count == 4);
    publish(5, 22);
    assert(advance_epoch(&boot, 0));
    assert(pulse_count == 5 && pulses[4] == 5 && snapshots == 1 && attempts == 2);
    assert(accepted == 1 && cs_idle(boot.file_card.service));
}

static void cancellation(int during) {
    setup(0);
    publish(1, 1); publish(2, 2); publish(3, 3);
    if (during) stop_at_pulse = 1;
    else stop_at_edge = 2;
    assert(advance_epoch(&boot, 0) && boot.epoch.last_sequence == 3);
    assert(edge_count == 3 && pulse_count == (during ? 1u : 0u));
    if (during) assert(pulses[0] == 1);
    host_stop_signal = 0; /* Controlled resume exposes a replay if one exists. */
    queue_read();
    assert(advance_epoch(&boot, 0) && !snapshots && !attempts);
    publish(4, 4);
    assert(advance_epoch(&boot, 0) && pulses[pulse_count-1] == 4);
    assert(pulse_count == (during ? 2u : 1u) && accepted == 1);
}

static void rejected(const char *scenario) {
    setup(0);
    queue_read();
    if (!strcmp(scenario, "early-wait")) {
        early_wait = 1;
        assert(!advance_epoch(&boot, 4));
        assert(!strcmp(boot.epoch.fault_reason, "host-wait-short"));
    } else if (!strcmp(scenario, "partial-refusal")) {
        reject_edge = 2;
        publish(1, 1); publish(2, 2); publish(3, 3);
        assert(!advance_epoch(&boot, 0));
        assert(boot.epoch.last_sequence == 1 && boot.epoch.pending_next == 1);
        assert(edge_count == 1 && !strcmp(boot.epoch.fault_reason, "vblank-edge"));
    } else {
        publish(1, 1); publish(1, 2);
        assert(!advance_epoch(&boot, 0));
        assert(!edge_count && !strcmp(boot.epoch.fault_reason, "source-edge"));
    }
    assert(boot.epoch.faulted && !boot.epoch.busy);
    assert(!advance_epoch(&boot, 0) && !pulse_count && !attempts && !snapshots);
}

static void wrong_owner(void) {
    setup(0);
    queue_read(); publish(1, 1);
    owner_id = 2;
    assert(!advance_epoch(&boot, 0) && !boot.epoch.faulted);
    assert(!edge_count && !pulse_count && host_cut.count == 1);
    owner_id = 1;
    assert(advance_epoch(&boot, 0) && pulse_count == 1 && accepted == 1);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    if (!strcmp(argv[1], "selected-guest")) selected_guest();
    else if (!strcmp(argv[1], "selected-host")) selected_host();
    else if (!strcmp(argv[1], "stop-before")) cancellation(0);
    else if (!strcmp(argv[1], "stop-during")) cancellation(1);
    else if (!strcmp(argv[1], "owner")) wrong_owner();
    else rejected(argv[1]);
    cs_destroy(boot.file_card.service);
    return 0;
}
